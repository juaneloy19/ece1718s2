#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>
#include <utility>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <string>

#include "util.h"
#include "matrix.h"
#include "frame.h"
#include "global_variable.h"

BYTEVEC_T read_byte_istream(std::istream& in)
{
	if(in.eof() || in.fail())
	{
		return BYTEVEC_T();
	}
	std::streampos orig_pos = in.tellg();
	
	in.seekg(0, std::ios_base::end);
	unsigned int size = (unsigned int)in.tellg() - orig_pos;
	BYTEVEC_T result(size);
	
	in.seekg(orig_pos);
	in.read(reinterpret_cast<char*>(&result[0]), size);
	return result;
}

BYTEVEC_T read_byte_file(const char* filename)
{
	std::ifstream ifs(filename, std::ifstream::binary);
	if (!ifs || !ifs.is_open())
	{
		std::cout << "Could not open file " << filename << std::endl;
		return BYTEVEC_T();
	}
	BYTEVEC_T ret = read_byte_istream(ifs);
	ifs.close();
	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cout << "Usage: encode.exe <path to cfg file> <input file name>" << std::endl;
		return 0;
	}
	
	const char* cfg_file = argv[1];
	CFG::inst().init(cfg_file);
	
	unsigned int num_frames, frame_width, frame_height;
	unsigned int block_size, search_range, qp;
	
	bool all_values_loaded = true;
	CFG_LOAD_OPT_MANDATORY("num_frames", num_frames, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("frame_width", frame_width, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("frame_height", frame_height, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("block_size", block_size, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("search_range", search_range, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("qp", qp, all_values_loaded)

	if(!all_values_loaded)
	{
		std::cout << "Missing options from config file " << cfg_file << std::endl;
		return 0;
	}
	
	BYTEVEC_T bytes = read_byte_file(argv[2]);
	
	std::cout << "INFO: Frames: " << frame_width << "x" << frame_height << ", Blocks: " << block_size << "x" << block_size << ", r=" << search_range << " qp=" << qp << std::endl;
	int bytes_per_frame = frame_width * frame_height /* y values */ + frame_width * frame_height / 4 /* u values */ + frame_width * frame_height / 4 /* v values */;
	std::cout << "INFO: Bytestring is " << bytes.size() << " bytes. Expected: " << num_frames << " frames * " << bytes_per_frame << " bytes/frame = " << bytes_per_frame * num_frames << std::endl;
	if (bytes.size() % bytes_per_frame != 0)
	{
			std::cout << "Error: Unexpected bytestream length; " << bytes.size() % bytes_per_frame << " extra bytes!" << std::endl;
			return 0;
	}
	
	
	bool debug_csv, debug_res_est;
	CFG_LOAD_OPT_DEFAULT("debug_csv", debug_csv, false);
	CFG_LOAD_OPT_DEFAULT("debug_res_est", debug_res_est, false);
	
	unsigned int max_refs, I_Period;
	CFG_LOAD_OPT_DEFAULT("nRefFrames", max_refs, 1);
	CFG_LOAD_OPT_DEFAULT("I_Period", I_Period, 1);
	
	if(debug_res_est)
	{
		DEBUG_CSV::push( std::vector<std::string>({ "Estimate", "Bytes Written", "SAD" }) );
	}

	bool dump_debug_files;
	CFG_LOAD_OPT_DEFAULT("dump_debug_files", dump_debug_files, false);
	
	clock_t overall_begin = std::clock();
	
	// Pull the bytes from the stream into separate frames
	std::vector<Frame> frames;
	for(auto it = bytes.begin(); it != bytes.end(); it += bytes_per_frame)
	{
		frames.push_back(Frame(BYTEVEC_T(it, it + bytes_per_frame), frame_width, frame_height));
	}
	std::cout << "INFO: Captured " << frames.size() << " frames!" << std::endl;
	assert(frames.size() == num_frames);
	
	std::ofstream real_outfile, ref_outfile, res_outfile, mvs_txt, res_txt;
	if(dump_debug_files)
	{
		real_outfile = std::ofstream("out.yuv", std::ofstream::binary);
		ref_outfile = std::ofstream("out_ref.yuv", std::ofstream::binary);
		res_outfile = std::ofstream("out_res.yuv", std::ofstream::binary);
		mvs_txt = std::ofstream ("mvs.txt");
		res_txt = std::ofstream ("res.txt");
	}
	
	std::ofstream mvs_ostream("mvs.db", std::ofstream::binary);
	std::ofstream res_ostream("res.db", std::ofstream::binary);
	std::ofstream recon_outfile("out_recon.yuv", std::ofstream::binary);
	
	{
		Frame temp_frame(0x80, frame_width, frame_height);
		temp_frame.pad_for_block_size(block_size);
		frame_width = temp_frame.get_width();
		frame_height = temp_frame.get_height();
		std::cout << "INFO: Padded frame to " << frame_width << "x" << frame_height << std::endl;
	}
	
	unsigned int iframe = 0;
	std::cout << std::setw(6) << "Frame";
	std::cout << std::setw(12) << "SAD" << std::setw(12) << "PSNR" << std::setw(12) << "SSIM" << std::setw(12) << "Bytes" << std::setw(12) << "Time";
	std::cout << std::endl;
	
	
	unsigned int P_PERIOD = I_Period - 1;
	unsigned int num_P_frames = P_PERIOD;
	
	unsigned int total_bytes_written = 0;
	double average_PSNR = 0.0;
	
	std::deque<Frame> ref_frames;
	
	for (auto cur_frame : frames)
	{	
		clock_t frame_begin = std::clock();
#ifdef JUAN_DEBUG
		std::string filename = "p_mb_info_" + std::to_string(iframe) + ".txt";
		std::string filename2 = "p_cache_rtl" + std::to_string(iframe) + ".txt";
		std::string filename3 = "p_cache_cmodel" + std::to_string(iframe) + ".txt";
		std::string filename7 = "p_C_cmodel" + std::to_string(iframe) + ".txt";
		std::string filename8 = "p_P_cmodel" + std::to_string(iframe) + ".txt";
		std::string filename9 = "p_P_prime_cmodel" + std::to_string(iframe) + ".txt";

		p_mb_info.open(filename);
		p_cache_rtl.open(filename2);
		p_cache_cmodel.open(filename3);
		p_C_cmodel.open(filename7);
		p_P_cmodel.open(filename8);
		p_P_prime_cmodel.open(filename9);

#endif
#ifdef DUMP_STIM
		std::string filename4 = "p_MEcur_block_rtl_" + std::to_string(iframe) + ".txt";
		std::string filename5 = "p_MEcache_rtl_rtl_" + std::to_string(iframe) + ".txt";
		std::string filename6 = "p_MEmv_rtl_" + std::to_string(iframe) + ".txt";
		p_MEcur_block_rtl.open(filename4);
		p_MEcache_rtl.open(filename5);
		p_MEmv_rtl.open(filename6);
#endif
		std::cout << std::setw(6) << iframe;
		cur_frame.pad_for_block_size(block_size);
		
		if(dump_debug_files)
			cur_frame.write(real_outfile, true);
		
		Frame recon_frame(0x80, frame_width, frame_height);
		unsigned int stream_bytes_written = 0;
		if(num_P_frames == P_PERIOD)
		{
			IFrame ifr(cur_frame, block_size, qp);
			
			if(dump_debug_files)
			{
				ifr.mode_frame().write(ref_outfile);
				ifr.res_frame().write(res_outfile);
			}
			
			// Signal that the next frame is an IFrame
			mvs_ostream.put(IFRAME_ID);
			stream_bytes_written += 1;
			stream_bytes_written = ifr.write(mvs_ostream, res_ostream);
			
			if(dump_debug_files)
			{
				mvs_txt << "Frame " << iframe << ":";
				res_txt << "Frame " << iframe << ":";
				ifr.print(mvs_txt, res_txt);
			}
			
			recon_frame = Frame(ifr);
			num_P_frames = 0;
			ref_frames.clear();
		}
		else
		{
			PFrame pf(cur_frame, ref_frames, block_size, search_range, qp);
			
			if(dump_debug_files)
			{
				pf.mv_frame(ref_frames).write(ref_outfile);
				pf.res_frame().write(res_outfile);
			}
			
			// Signal that the next frame is a PFrame
			mvs_ostream.put(PFRAME_ID);
			stream_bytes_written += 1;
			stream_bytes_written = pf.write(mvs_ostream, res_ostream);
			
			if(dump_debug_files)
			{
				mvs_txt << "Frame " << iframe << ":";
				res_txt << "Frame " << iframe << ":";
				pf.print(mvs_txt, res_txt);
			}
			
			recon_frame = Frame(pf, ref_frames);
			++num_P_frames;
		}
		
		// We need to construct the reconstructed frame as a reference anyway, so dump it
		// so that we can diff it vs. the decoded video later.
		recon_frame.write(recon_outfile, false);
		
		// Collect and dump debug statistics
		unsigned int SAD = cur_frame.SAD(recon_frame);
		double PSNR = cur_frame.PSNR(recon_frame);
		double SSIM = cur_frame.SSIM(recon_frame);
		clock_t frame_end = std::clock();
		std::cout << std::setw(12) << SAD;
		std::cout << std::setw(12) << PSNR;
		std::cout << std::setw(12) << SSIM;
		std::cout << std::setw(12) << stream_bytes_written;
		std::cout << std::setw(12) << double(frame_end - frame_begin) / CLOCKS_PER_SEC;
		std::cout << std::endl;
		total_bytes_written += stream_bytes_written;
		average_PSNR += PSNR;
		
		ref_frames.push_front(recon_frame);
		if(ref_frames.size() > max_refs)
		{
			ref_frames.pop_back();
		}
		
		++iframe;
#ifdef JUAN_DEBUG
		p_mb_info.close();
		p_cache_rtl.close();
		p_cache_cmodel.close();
		p_C_cmodel.close();
		p_P_cmodel.close();
		p_P_prime_cmodel.close();
#endif

#ifdef DUMP_STIM
		p_MEcur_block_rtl.close();
		p_MEcache_rtl.close();
		p_MEmv_rtl.close();
#endif
	}
	std::cout << std::endl;
	real_outfile.close();
	ref_outfile.close();
	res_outfile.close();
	recon_outfile.close();
	mvs_ostream.close();
	res_ostream.close();
	
	clock_t overall_end = std::clock();
	std::cout << "Total_Time:" << std::setw(12) << double(overall_end - overall_begin) / CLOCKS_PER_SEC << std::endl;
	std::cout << "Total_Bytes:" << std::setw(12) << total_bytes_written << std::endl;
	std::cout << "Average_PSNR:" << std::setw(12) << average_PSNR / (double)num_frames << std::endl;
	if(debug_csv || debug_res_est)
	{
		if(debug_res_est)
			DEBUG_CSV::push(std::vector<std::string>({ "" }));
		DEBUG_CSV::push( std::vector<std::string>({ "Total Time", std::to_string(double(overall_end - overall_begin) / CLOCKS_PER_SEC ) }) );
		DEBUG_CSV::push( std::vector<std::string>({ "Total Bytes", std::to_string(total_bytes_written) }) );
		DEBUG_CSV::push( std::vector<std::string>({ "Average PSNR", std::to_string(average_PSNR / (double)num_frames) }) );
	}
}