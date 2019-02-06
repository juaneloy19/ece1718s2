#include <iostream>
#include <deque>
#include  <iomanip>

#include "matrix.h"
#include "residual.h"

#ifndef _FRAME_H
#define _FRAME_H

extern std::ofstream p_mb_info;
extern std::ofstream p_cache_rtl;
extern std::ofstream p_cache_cmodel;

extern std::ofstream p_MEcur_block_rtl;
extern std::ofstream p_MEcache_rtl;
extern std::ofstream p_MEmv_rtl;

extern std::ofstream p_C_cmodel;
extern std::ofstream p_P_cmodel;
extern std::ofstream p_P_prime_cmodel;

extern std::ofstream p_PeCost;




/* Forward declaration of PFrame and an identifier to use in the bytestreams */
class PFrame;
const BYTE_T PFRAME_ID = 0x00;

/* Forward declaration of IFrame */
class IFrame;
const BYTE_T IFRAME_ID = 0x01;

class Frame
{
public:
	/* constructor from raw bytes */
	Frame(BYTEVEC_T bytes, unsigned int width, unsigned int height, bool y_only=false);
	
	/* Constructor from an array of y-value only blocks */
	Frame(BLOCKVEC_T y_blocks, unsigned int block_size, unsigned int width, unsigned int height);
	
	/* Constructor from an array of y-value only blocks coloured by the caller in some way */
	Frame(BLOCKVEC_T y_blocks, unsigned int block_size, unsigned int width, unsigned int height, const INT_VEC_T& block_colours);
	
	/* Constructor from a frame of residual blocks; Residual Frames are useful for debugging */
	Frame(std::vector<ResidualBlock> residuals, unsigned int block_size, unsigned int width, unsigned int height);
	
	/* Constructor from a PFrame and its corresponding reference frame */
	Frame(const PFrame& pf, const std::deque<Frame>& refs);
	
	/* Constructor from an IFrame */
	Frame(const IFrame& pf);
	
	/* Constructor from a Y-Value Byte, width, and height */
	Frame(BYTE_T y_byte, unsigned int width, unsigned int height)
	: 	m_width(width),
		m_height(height),
		y_values(y_byte, width, height),
		u_values(0x80, width/2, height/2),
		v_values(0x80, width/2, height/2)
		{};
	
	void write(std::ostream& out, bool write_y_only = false);
	void print(std::ostream& out, bool print_y_only = false);
	
	void pad_for_block_size(unsigned int i);
	void pad_width(unsigned int n);
	void pad_height(unsigned int n);

	unsigned int get_width() 	const { return m_width; }
	unsigned int get_height() 	const { return m_height; }
	
	ByteMatrix get_y_block_at(COORD_T coord, unsigned int i) const;
	std::vector<COORD_T> get_y_block_coords(unsigned int i) const;
	BLOCKVEC_T get_y_block_vec(unsigned int i) const;
	bool block_coord_is_legal(COORD_T coord, unsigned int i) const;
	
	double MSE(const Frame& rhs) 	const { return y_values.MSE(rhs.y_values); }
	double PSNR(const Frame& rhs) 	const { return y_values.PSNR(rhs.y_values); }
	double SSIM(const Frame& rhs) 	const { return y_values.SSIM(rhs.y_values); }
	double SAD(const Frame& rhs) 	const { return y_values.SAD(rhs.y_values); }

private:
	void init_from_y_blocks(const BLOCKVEC_T& blocks, unsigned int block_size, unsigned int width, unsigned int height, const INT_VEC_T& block_colours);

	unsigned int m_width;
	unsigned int m_height;
	ByteMatrix y_values;
	ByteMatrix u_values;
	ByteMatrix v_values;
};

struct MV_T
{
	int x;
	int y;
	int i;
	MV_T(int ix, int iy, int ii) : x(ix), y(iy), i(ii) {}; 
	MV_T() : x(0), y(0), i(0) {}; 
	
	bool operator==(const MV_T& rhs) { return (x == rhs.x) && (y == rhs.y) && (i == rhs.i); }
};

typedef std::pair< MV_T, ResidualBlock > PF_REF_T;
typedef std::vector< PF_REF_T > PF_REF_VEC_T;

class PFrame
{
public:
	/* Encoder-side constructor; we have a current frame to encode and a reference frame to base it on */
	PFrame(const Frame& cur_frame, const std::deque<Frame>& ref_frames, unsigned int i, int r, unsigned int qp);
	
	/* Decoder-side constructor; we have a streams that the encoder-side PFrame wrote to how big the frame is */
	PFrame(std::istream& mv_in, std::istream& res_in, unsigned int i, unsigned int frame_width, unsigned int frame_height, unsigned int qp);

	unsigned int write(std::ostream& mv_out, std::ostream& res_out);
	void print(std::ostream& mv_out, std::ostream& res_out);
	
	const PF_REF_VEC_T& get_refs() 	const { return m_mv_and_residuals; }
	unsigned int get_block_size() 	const { return m_block_size; }
	INT_VEC_T get_block_colours()		const;
	
	Frame res_frame();
	Frame mv_frame(const std::deque<Frame>& ref_frames);
	
	static COORD_T mv_to_coord(MV_T mv, COORD_T base_coord)
	{
		COORD_T ret;
		ret.first = 		static_cast<unsigned int>( static_cast<int>(base_coord.first) + mv.y );
		ret.second = 	static_cast<unsigned int>( static_cast<int>(base_coord.second) + mv.x);
		return ret;
	}
	
	static MV_T coords_to_mv(COORD_T base_coord, COORD_T dest_coord)
	{
		MV_T ret;
		ret.y = 		static_cast<int>(dest_coord.first) - static_cast<int>(base_coord.first);
		ret.x =	static_cast<int>(dest_coord.second) - static_cast<int>(base_coord.second);
		return ret;
	}
	
	static unsigned int dx_plus_dy(MV_T mv)
	{
		return abs(mv.x) + abs(mv.y);
	}
//Juan
	static int GetCachePos(int cur_pos, int r, int limit, int window_width, int block_size);
	static int ME(ByteMatrix a, ByteMatrix b, int block_size, int offset_x, int offset_y);
//	static std::pair<int, int> PFrame::StartME(ByteMatrix cache, ByteMatrix cur_block, SearchQ search_vectors, int cache_width, int cache_height, int block_size);
//

private:

	static std::tuple<unsigned int, ByteMatrix, MV_T> search_for_best_ref (
		const COORD_T& cur_coord, 
		const ByteMatrix& cur_block,
		const std::deque<Frame>& ref_frames, 
		int r, 
		unsigned int block_size,
		unsigned int qp,
		bool fast_me,
		const MV_T& last_mv );
//Juan
	static std::tuple<unsigned int, ByteMatrix, MV_T> search_for_best_ref_hw(
		const COORD_T& cur_coord,
		const ByteMatrix& cur_block,
		const std::deque<Frame>& ref_frames,
		int r,
		unsigned int block_size,
		unsigned int qp,
		bool fast_me,
		const MV_T& last_mv);

	unsigned int m_block_size;
	unsigned int m_frame_width;
	unsigned int m_frame_height;
	PF_REF_VEC_T m_mv_and_residuals;
};
//
typedef int INTRA_MODE_T;
typedef std::pair< INTRA_MODE_T, ResidualBlock > IF_REF_T;
typedef std::vector< IF_REF_T > IF_REF_VEC_T;

class IFrame
{
public:
	IFrame(const Frame& cur_frame, unsigned int i, unsigned int qp);
	
	IFrame(std::istream& mode_in, std::istream& res_in, unsigned int i, unsigned int frame_width, unsigned int frame_height, unsigned int qp);
	
	unsigned int write(std::ostream& mode_out, std::ostream& res_out);
	void print(std::ostream& mode_out, std::ostream& res_out);
	
	const BLOCKVEC_T& get_recon_blocks()		const { return m_recon_blocks; }
	INT_VEC_T get_block_colours()		const;
	
	unsigned int get_block_size()			const { return m_block_size; }
	
	unsigned int get_frame_height() const { return m_frame_height; }
	unsigned int get_frame_width()  const { return m_frame_width; }
	
	Frame res_frame() const;
	Frame mode_frame()						const { return Frame(m_ref_blocks, m_block_size, m_frame_width, m_frame_height, get_block_colours()); };
	
private:
	unsigned int m_block_size;
	unsigned int m_frame_width;
	unsigned int m_frame_height;
	BLOCKVEC_T m_recon_blocks;
	BLOCKVEC_T m_ref_blocks;
	IF_REF_VEC_T m_modes_and_residuals;
};

#endif //_FRAME_H