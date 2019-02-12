#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>
#include <utility>
#include <iomanip>
#include <cmath>
#include <ctime>

#include "matrix.h"
#include "frame.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: decode.exe <path to cfg file> <mvs db file> <res db file>" << std::endl;
		return 0;
	}
	
	const char* cfg_file = argv[1];
	CFG::inst().init(cfg_file);
	
	unsigned int num_frames, frame_width, frame_height;
	unsigned int block_size, qp;
	bool all_values_loaded = true;
	CFG_LOAD_OPT_MANDATORY("num_frames", num_frames, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("frame_width", frame_width, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("frame_height", frame_height, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("block_size", block_size, all_values_loaded)
	CFG_LOAD_OPT_MANDATORY("qp", qp, all_values_loaded)
	
	std::string mvs_filename(argv[2]);
	std::string res_filename(argv[3]);

	if(!all_values_loaded)
	{
		std::cout << "Missing options from config file " << cfg_file << std::endl;
		return 0;
	}
	
	{
		Frame temp_frame(0x80, frame_width, frame_height);
		temp_frame.pad_for_block_size(block_size);
		frame_width = temp_frame.get_width();
		frame_height = temp_frame.get_height();
	}
	
	unsigned int max_refs;
	CFG_LOAD_OPT_DEFAULT ("nRefFrames", max_refs, 1)
		
	std::ifstream mvs_db(mvs_filename, std::ifstream::binary);
	std::ifstream res_db(res_filename, std::ifstream::binary);
	
	std::ofstream out("out_decode.yuv", std::ofstream::binary);

	unsigned int iframe = 0;
	char frame_type;
	
	clock_t overall_begin = std::clock();
	
	std::deque<Frame> ref_frames;
	
	while (mvs_db.good() && !mvs_db.eof() && mvs_db.peek() != EOF && res_db.good() && !res_db.eof() && res_db.peek() != EOF)
	{
		std::cout << "Decoding Frame " << iframe++ << "..." << std::flush;
		Frame decode_frame(0x80, frame_width, frame_height);
		
		clock_t frame_begin = std::clock();
		
		mvs_db.get(frame_type);
		assert(frame_type == IFRAME_ID || frame_type == PFRAME_ID);
		if (frame_type == IFRAME_ID)
		{
			IFrame ifr(mvs_db, res_db, block_size, frame_width, frame_height, qp);
			decode_frame = Frame(ifr);
			ref_frames.clear();
		}
		else
		{
			PFrame pf(mvs_db, res_db, block_size, frame_width, frame_height, qp);
			decode_frame = Frame(pf, ref_frames);
		}
		decode_frame.write(out);
		ref_frames.push_front(decode_frame);
		if(ref_frames.size() > max_refs)
			ref_frames.pop_back();
		
		clock_t frame_end = std::clock();
		std::cout << " Elapsed time: "  << double(frame_end - frame_begin) / CLOCKS_PER_SEC << std::endl;
	}
	out.close();
	mvs_db.close();
	res_db.close();
	
	if(iframe != num_frames)
	{
		std::cout << "ERROR: Expected " << num_frames << " frames, only decoded " << iframe << std::endl;
	}
	
	clock_t overall_end = std::clock();
	std::cout << "Overall_Time:" << std::setw(12) << double(overall_end - overall_begin) / CLOCKS_PER_SEC << std::endl;
}