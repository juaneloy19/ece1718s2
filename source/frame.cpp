#include <cassert>
#include <iomanip>
#include <queue>
#include <map>
#include <tuple>
#include <limits>

#include "frame.h"

COORD_T calculate_next_coord(const COORD_T& cur_coord, unsigned int full_block_size, unsigned int cur_block_size, unsigned int frame_width)
{
	COORD_T next_coord = cur_coord;
	if(cur_block_size == full_block_size)
	{
		next_coord.second += full_block_size;
	}
	else
	{
		assert(cur_block_size == full_block_size/2);
		
		if(next_coord.second % full_block_size == 0)
		{
			next_coord.second += cur_block_size;
		}
		else if (next_coord.first % full_block_size == 0)
		{
			next_coord.first  += cur_block_size;
			next_coord.second -= cur_block_size;
		}
		else
		{
			next_coord.first  -= cur_block_size;
			next_coord.second += cur_block_size;	
		}
	}
	if(next_coord.second == frame_width)
	{
		next_coord.first += full_block_size;
		next_coord.second = 0;
	}
	return next_coord;
}

Frame::Frame(BYTEVEC_T bytes, unsigned int width, unsigned int height, bool y_only) :
m_width(width),
m_height(height)
{
	if (!y_only)
	{
		assert (bytes.size() == (m_width*height + m_width*m_height/4 + m_width*m_height/4));
	}
	else
	{
		assert (bytes.size() == (m_width*height));
	}
	
	auto it = bytes.begin();
	y_values = ByteMatrix(BYTEVEC_T(it, it + m_width*m_height), m_width, m_height);
	it += m_width*m_height;
	
	if (y_only)
	{
		u_values = ByteMatrix(BYTEVEC_T(m_width*m_height/4, 0x80), m_width/2, m_height/2);
		v_values = ByteMatrix(BYTEVEC_T(m_width*m_height/4, 0x80), m_width/2, m_height/2);
	}
	else
	{
		u_values = ByteMatrix(BYTEVEC_T(it, it + m_width*m_height/4), m_width/2, m_height/2);
		it += m_width*m_height/4;
		
		v_values = ByteMatrix(BYTEVEC_T(it, it + m_width*m_height/4), m_width/2, m_height/2);
		it += m_width*m_height/4;
	}
	assert(it == bytes.end());
}

Frame::Frame(BLOCKVEC_T y_blocks, unsigned int block_size, unsigned int width, unsigned int height) 
{
	init_from_y_blocks(y_blocks, block_size, width, height, INT_VEC_T(y_blocks.size(), 0)); 
}

Frame::Frame(BLOCKVEC_T y_blocks, unsigned int block_size, unsigned int width, unsigned int height, const INT_VEC_T& block_colours) 
{
	init_from_y_blocks(y_blocks, block_size, width, height, block_colours); 
}

Frame::Frame(std::vector<ResidualBlock> residuals, unsigned int block_size, unsigned int width, unsigned int height)
{
	COORD_T block_coord({0, 0});
	BLOCKVEC_T res_blocks;
	for(auto& res : residuals)
	{
		res_blocks.push_back(BLOCK_T(block_coord, res.as_y_block()));
		block_coord = calculate_next_coord(block_coord, block_size, res.get_block_size(), width);
	}
	assert(block_coord.first == height);
	init_from_y_blocks(res_blocks, block_size, width, height, INT_VEC_T(res_blocks.size(), 0));
}

Frame::Frame(const PFrame& pf, const std::deque<Frame>& refs)
{
	unsigned int block_size = pf.get_block_size();
	BLOCKVEC_T recon_blocks;

	unsigned int frame_width  = refs[0].get_width();
	unsigned int frame_height = refs[0].get_height();
	
	auto pf_refs = pf.get_refs();
	COORD_T block_coord({0, 0});
	
	unsigned int i = 0;
	while(block_coord.first < frame_height)
	{
		MV_T ref_mv = pf_refs[i].first;
		COORD_T ref_coord = PFrame::mv_to_coord(ref_mv, block_coord);
		unsigned int iref = (unsigned int)ref_mv.i;
		assert(iref < refs.size());
	
		auto ref_block = refs[iref].get_y_block_at(ref_coord, pf_refs[i].second.get_block_size());
		ByteMatrix recon_block = pf_refs[i].second.reconstruct_from(ref_block);
		
		recon_blocks.push_back(BLOCK_T(block_coord, recon_block));
		
		block_coord = calculate_next_coord(block_coord, block_size, pf_refs[i].second.get_block_size(), frame_width);
		++i;
	}
	assert(block_coord.first == frame_height);
	
	init_from_y_blocks(recon_blocks, block_size, frame_width, frame_height, pf.get_block_colours());
}

Frame::Frame(const IFrame& ifr) 
{
	init_from_y_blocks(ifr.get_recon_blocks(), ifr.get_block_size(), ifr.get_frame_width(), ifr.get_frame_height(), ifr.get_block_colours());	
}

BYTE_T u_colour(const int icolour)
{
	static const BYTEVEC_T u_colours = { 0x80, 0x1F, 0x1F, 0xE0, 0xE0 };
	int id = std::max(icolour, 0);
	id = std::min(icolour, 4);
	return u_colours[id];
}

BYTE_T v_colour(const int icolour)
{
	static const BYTEVEC_T v_colours = { 0x80, 0x1F, 0xE0, 0x1F, 0xE0 };
	int id = std::max(icolour, 0);
	id = std::min(icolour, 4);
	return v_colours[id];
}

ByteMatrix get_u_block(const int icolour, const unsigned int block_size)
{
	bool colour_blocks;
	CFG_LOAD_OPT_DEFAULT("debug_colour_blocks", colour_blocks, false);
	
	ByteMatrix ret(0x80, block_size, block_size);
	if(colour_blocks)
	{
		ret = ByteMatrix::generate_border_block(u_colour(icolour), 0xFF, block_size, block_size);
	}
	return ret;
}

ByteMatrix get_v_block(const int icolour, const unsigned int block_size)
{
	bool colour_blocks;
	CFG_LOAD_OPT_DEFAULT("debug_colour_blocks", colour_blocks, false);
	
	ByteMatrix ret(0x80, block_size, block_size);
	if(colour_blocks)
	{
		ret = ByteMatrix::generate_border_block(v_colour(icolour), 0x40, block_size, block_size);
	}
	return ret;
}

void Frame::init_from_y_blocks(const BLOCKVEC_T& y_blocks, unsigned int block_size, unsigned int width, unsigned int height, const INT_VEC_T& block_colours)
{
	assert(block_colours.size() == y_blocks.size());
	m_width = width;
	m_height = height;
	
	unsigned int i = 0;
	auto iblock = y_blocks.begin();
	while (iblock != y_blocks.end())
	{
		ByteMatrix y_block_row;
		ByteMatrix u_block_row;
		ByteMatrix v_block_row;
		while(y_block_row.get_width() < m_width)
		{
			assert(iblock != y_blocks.end());
			
			COORD_T iblock_coord = iblock->first;
			ByteMatrix y_block = iblock->second;
			assert(iblock_coord.second == y_block_row.get_width());
			assert(y_block.get_width() == y_block.get_height());
			assert(y_block.get_width()  > 0 && y_block.get_width()  % 2 == 0);
			assert(y_block.get_height() > 0 && y_block.get_height() % 2 == 0);
			
			unsigned int uv_block_size = y_block.get_width() / 2;
			ByteMatrix u_block = get_u_block(block_colours[i], uv_block_size);
			ByteMatrix v_block = get_v_block(block_colours[i], uv_block_size);
			
			if(y_block.get_width() == block_size/2)
			{
				//The next three blocks form a Z-pattern to reconstruct the whole block
				
				//First the top right...
				++i;
				iblock++;
				assert(iblock != y_blocks.end());
				
				ByteMatrix top_right_block = iblock->second;
				y_block.stitch_right( top_right_block );
				u_block.stitch_right( get_u_block(block_colours[i], uv_block_size) );
				v_block.stitch_right( get_v_block(block_colours[i], uv_block_size) );
				
				//... next the bottom left...
				++i;
				iblock++;
				assert(iblock != y_blocks.end());
				
				ByteMatrix lower_y_block = iblock->second;
				ByteMatrix lower_u_block = get_u_block(block_colours[i], uv_block_size);
				ByteMatrix lower_v_block = get_v_block(block_colours[i], uv_block_size);
				
				//... and finally the bottom right
				++i;
				iblock++;
				assert(iblock != y_blocks.end());
				
				lower_y_block.stitch_right( iblock->second );
				lower_u_block.stitch_right( get_u_block(block_colours[i], uv_block_size) );
				lower_v_block.stitch_right( get_v_block(block_colours[i], uv_block_size) );
				
				y_block.stitch_below( lower_y_block );
				u_block.stitch_below( lower_u_block );
				v_block.stitch_below( lower_v_block );
			}
			else
			{
				assert(y_block.get_width() == block_size);
			}
			
			y_block_row.stitch_right( y_block );
			u_block_row.stitch_right( u_block );
			v_block_row.stitch_right( v_block );
			
			++i;
			iblock++;
		}
		y_values.stitch_below(y_block_row);
		u_values.stitch_below(u_block_row);
		v_values.stitch_below(v_block_row);
		assert(iblock == y_blocks.end() || (*iblock).first.first == y_values.get_height());
	}
	assert(y_values.get_height() == m_height);
	assert(y_values.get_width() == m_width);
	assert(u_values.get_height() == m_height/2);
	assert(u_values.get_width() == m_width/2);
	assert(v_values.get_height() == m_height/2);
	assert(v_values.get_width() == m_width/2);
}

void Frame::pad_width(unsigned int n)
{
	assert(n % 2 == 0);
	y_values.pad_width(n, 0x80);
	u_values.pad_width(n/2, 0x80);
	v_values.pad_width(n/2, 0x80);
	m_width += n;
}

void Frame::pad_height(unsigned int n)
{
	assert(n % 2 == 0);
	y_values.pad_height(n, 0x80);
	u_values.pad_height(n/2, 0x80);
	v_values.pad_height(n/2, 0x80);
	m_height += n;
}

void Frame::write(std::ostream& out, bool write_y_only)
{
	out.write(reinterpret_cast<char*>(&y_values.as_vec()[0]), y_values.as_vec().size());
	
	if (write_y_only)
	{
		out.write(reinterpret_cast<char*>(&BYTEVEC_T(u_values.as_vec().size(), 0x80)[0]), u_values.as_vec().size());
		out.write(reinterpret_cast<char*>(&BYTEVEC_T(v_values.as_vec().size(), 0x80)[0]), v_values.as_vec().size());
	}
	else
	{
		out.write(reinterpret_cast<char*>(&u_values.as_vec()[0]), u_values.as_vec().size());
		out.write(reinterpret_cast<char*>(&v_values.as_vec()[0]), v_values.as_vec().size());
	}
}

void Frame::print(std::ostream& out, bool print_y_only)
{
	out << "Y-Values" << std::endl;
	y_values.print(out);
	if (!print_y_only)
	{
		out << "\n\nU-Values" << std::endl;
		u_values.print(out);
		out << "\n\nV-Values" << std::endl;
		v_values.print(out);
	}
}

ByteMatrix Frame::get_y_block_at(COORD_T coord, unsigned int i) const
{
	assert(y_values.block_coord_is_legal(coord, i, true));
	
	return y_values.get_block_at(coord, i);
}

std::vector<COORD_T> Frame::get_y_block_coords(unsigned int i) const
{
	return y_values.get_block_coords(i);
}

bool Frame::block_coord_is_legal(COORD_T coord, unsigned int i) const
{
	return y_values.block_coord_is_legal(coord, i);
}

void Frame::pad_for_block_size(unsigned int i)
{
	if (m_width % i != 0)
	{
		pad_width(i - (m_width % i));
	}
	if(m_height % i != 0)
	{
		pad_height(i - (m_height % i));
	}
}

BLOCKVEC_T Frame::get_y_block_vec(unsigned int i) const
{
	if (m_width % i != 0 || m_height % i != 0)
	{
		std::cout << "ERROR: Retrieiving y_block_vec for blocks of size " << i << "x" << i << " when Frame is size " << m_width << "x" << m_height << std::endl;
	}
	assert(m_width % i == 0);
	assert(m_height % i == 0);
	
	BLOCKVEC_T ret;
	std::vector<COORD_T> coords = get_y_block_coords(i);
	for (auto& c : coords)
	{
		ret.push_back(BLOCK_T(c, get_y_block_at(c, i)));
	}
	return ret;
}

//**************************************************************************
// BEGIN PFRAME
//**************************************************************************

class SearchQ
{
public:
	SearchQ() {}
	
	void push(const std::pair<int, int>& v)
	{
		if(m_vecs_pushed.find(v) == m_vecs_pushed.end())
		{
			m_vecs_to_search.push(v);
			m_vecs_pushed[v] = true;
		}
	}
	
	std::pair<int, int> pop()
	{
		std::pair<int, int> ret = m_vecs_to_search.front();
		m_vecs_to_search.pop();
		return ret;
	}
	
	bool empty() { return m_vecs_to_search.empty(); }
	
private:
	std::queue < std::pair<int, int> > m_vecs_to_search;
	std::map < std::pair<int, int>, bool > m_vecs_pushed;
};

std::tuple<unsigned int, ByteMatrix, MV_T> PFrame::search_for_best_ref (
	const COORD_T& cur_coord, 
	const ByteMatrix& cur_block,
	const std::deque<Frame>& ref_frames, 
	int r, 
	unsigned int block_size,
	unsigned int qp,
	bool fast_me,
	const MV_T& last_mv ) 
{
	static int search_i, search_j, i_x, i_y;
	
	unsigned int min_cost = 0;
	ByteMatrix best_ref_block;
	MV_T res_mv;
	
	for(unsigned int iref = 0; iref < ref_frames.size(); ++iref)
	{
		SearchQ search_vectors;
		
		if(fast_me)
		{
			// Search in a cross around 0,0; and also the previous mv
			for (i_x = 3; i_x <= r; i_x += 3)
			{
				search_vectors.push(std::make_pair( 0, i_x ) );
				search_vectors.push(std::make_pair( 0, -i_x ) );
			}
			for (i_y = 3; i_y <= r; i_y += 3)
			{
				search_vectors.push(std::make_pair( i_y, 0 ) );
				search_vectors.push(std::make_pair( -i_y, 0  ) );
			}
			search_vectors.push(std::make_pair( 0, 0 ) );
			search_vectors.push(std::make_pair( (int)last_mv.y, (int)last_mv.x ) );
		}
		else
		{
			for(search_i = -r; search_i <= r; ++search_i)
			{
				for(search_j = -r; search_j <= r; ++search_j)
				{
					search_vectors.push( std::make_pair(search_i, search_j) );
				}
			}
		}
		
		while(!search_vectors.empty())
		{
			std::tie(search_i, search_j) = search_vectors.pop();
			
			if (int(cur_coord.first) + search_i < 0 || int(cur_coord.second) + search_j < 0)
			{
				continue;
			}
			
			COORD_T search_coord(cur_coord.first + search_i, cur_coord.second + search_j);
			if(!ref_frames[iref].block_coord_is_legal(search_coord, block_size))
			{
				continue;
			}
			
			ByteMatrix ref_block = ref_frames[iref].get_y_block_at(search_coord, block_size);
			MV_T search_mv = PFrame::coords_to_mv(cur_coord, search_coord);
			unsigned int mv_bytes = (search_mv == last_mv)? 0 : sizeof(MV_T);
			unsigned int cost = ResidualBlock::estimate_rd_cost(cur_block, ref_block, qp, mv_bytes);
			
			if 	( 	( best_ref_block.get_width() == 0 || best_ref_block.get_height() == 0) ||
					( cost <   min_cost ) ||
					( cost == min_cost && (PFrame::dx_plus_dy(search_mv) <   PFrame::dx_plus_dy(res_mv)) ) ||
					( cost == min_cost && (PFrame::dx_plus_dy(search_mv) == PFrame::dx_plus_dy(res_mv)) && (abs(search_mv.x) < abs(res_mv.x))	) ||
					( cost == min_cost && (PFrame::dx_plus_dy(search_mv) == PFrame::dx_plus_dy(res_mv)) && (abs(search_mv.x) == abs(res_mv.x)) && (abs(search_mv.y) < abs(res_mv.y))	)
				)
			{
				best_ref_block = ref_block;
				
				res_mv = search_mv;
				res_mv.i = iref;
				
				// Add nearest neighbours if they're not already being searched
				search_vectors.push(std::make_pair( std::max(-r, search_i-1), search_j ) );
				search_vectors.push(std::make_pair( std::min( r, search_i+1), search_j ) );
				search_vectors.push(std::make_pair( search_i, std::max(-r, search_j-1) ) );
				search_vectors.push(std::make_pair( search_i, std::min( r, search_j+1) ) );
				
				min_cost = cost;
			}
			
		}
	}
	return std::make_tuple(min_cost, best_ref_block, res_mv);
}

int PFrame::GetCachePos(int cur_pos, int r , int limit, int window_width, int block_size) {
	int offset = 0;
	for (int i = 1; i <= (r/2); i++) {//Check neg direction
		if ((cur_pos - i ) <= 0) {
			return 0;
		}
	}
	for (int i = 1; i <= (r/2); i++) {//Check pos direction
		if ((cur_pos + (window_width/2) + i) >= limit) {
			return limit - window_width ;
//		return limit - window_width + 1;
		}
	}
//	return cur_pos - (block_size / 2) ;

	return cur_pos- (block_size/2) + 1;

}
int PFrame::ME(ByteMatrix a, ByteMatrix b, int block_size, int offset_x, int offset_y) {
	int latch1 = 0;
	int latch2 = 0;
	int latch3 = 0;
	int PE = 0;
	int c;
	for (int i = 0; i < block_size; i++) {
		for (int j = 0; j < block_size; j++) {
			c = b[i+offset_y][j + offset_x];
			latch1 = a[i][j] - b[i+offset_y][j + offset_x];
			latch2 = abs(latch1);
			latch3 = latch3 + latch2;
#ifdef JUAN_DEBUG
			p_Acumm << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(latch3) << " ";
#endif
		}
	}
	p_Acumm << "\n";

	PE = latch3;
	return PE;
}

std::pair<int, int> StartME(ByteMatrix Host_cache, ByteMatrix cur_block, SearchQ search_vectors, int cache_width, int cache_height, int block_size, int a, int b) {
	int PE[16] = { 0 };
	int BestCost = 99999999;
	std::pair<int, int> BestMV;
	std::pair<int, int> MV;
	ByteMatrix ME_cache(0, cache_height, cache_width);
	ByteMatrix ME_Cur_Block(0, block_size, block_size);
#ifdef DUMP_STIM
	p_MEcur_block_rtl << "MB_Y: " << a / block_size << " MB_X: " << b / block_size << "\n";
#endif 
	//LOAD CUR BLOCK
	for (int i = 0; i < block_size; i++){
		for (int j = 0; j < block_size; j = j + 8) {
			ME_Cur_Block.m_matrix[i][j+0] = cur_block.m_matrix[i][j+0];
			ME_Cur_Block.m_matrix[i][j + 1] = cur_block.m_matrix[i][j + 1];
			ME_Cur_Block.m_matrix[i][j + 2] = cur_block.m_matrix[i][j + 2];
			ME_Cur_Block.m_matrix[i][j + 3] = cur_block.m_matrix[i][j + 3];
			ME_Cur_Block.m_matrix[i][j + 4] = cur_block.m_matrix[i][j + 4];
			ME_Cur_Block.m_matrix[i][j + 5] = cur_block.m_matrix[i][j + 5];
			ME_Cur_Block.m_matrix[i][j + 6] = cur_block.m_matrix[i][j + 6];
			ME_Cur_Block.m_matrix[i][j + 7] = cur_block.m_matrix[i][j + 7];
#ifdef DUMP_STIM
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 0]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 1]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 2]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 3]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 4]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 5]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 6]) << " ";
			p_MEcur_block_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[i][j + 7]) << " ";
#endif 

		}
#ifdef DUMP_STIM
		p_MEcur_block_rtl << "\n";
#endif
	}

#ifdef DUMP_STIM
	p_MEcur_block_rtl << "\n";
	p_MEcache_rtl << "MB_Y: " << a / block_size << " MB_X: " << b / block_size << "\n";
#endif
	//LOAD CACHE
	for (int i = 0; i < cache_height; i++) {
		for (int j = 0; j < cache_width; j = j + 8) {
			ME_cache.m_matrix[i][j] = Host_cache.m_matrix[i][j];
			ME_cache.m_matrix[i][j + 1] = Host_cache.m_matrix[i][j + 1];
			ME_cache.m_matrix[i][j + 2] = Host_cache.m_matrix[i][j + 2];
			ME_cache.m_matrix[i][j + 3] = Host_cache.m_matrix[i][j + 3];
			ME_cache.m_matrix[i][j + 4] = Host_cache.m_matrix[i][j + 4];
			ME_cache.m_matrix[i][j + 5] = Host_cache.m_matrix[i][j + 5];
			ME_cache.m_matrix[i][j + 6] = Host_cache.m_matrix[i][j + 6];
			if (j + 7< cache_width)//Corner case cache width is -1
				ME_cache.m_matrix[i][j + 7] = Host_cache.m_matrix[i][j + 7];

			#ifdef DUMP_STIM
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 0]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 1]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 2]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 3]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 4]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 5]) << " ";
			p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 6]) << " ";
			if(j+7< cache_width)
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 7]) << " ";
			else
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(255) << " ";

			#endif

			/*Simplified code above since we are assuming cache width is aligned to 8
			if (j < cache_width) {
				ME_cache.m_matrix[i][j] = Host_cache.m_matrix[i][j];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 0]) << " ";
#endif
			}
			if (j + 1 < cache_width) {
				ME_cache.m_matrix[i][j + 1] = Host_cache.m_matrix[i][j + 1];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 1]) << " ";
#endif
			}
			if (j + 2 < cache_width) {
				ME_cache.m_matrix[i][j + 2] = Host_cache.m_matrix[i][j + 2];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 2]) << " ";
#endif
			}
			if (j + 3 < cache_width) {
				ME_cache.m_matrix[i][j + 3] = Host_cache.m_matrix[i][j + 3];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 3]) << " ";
#endif
			}
			if (j + 4 < cache_width) {
				ME_cache.m_matrix[i][j + 4] = Host_cache.m_matrix[i][j + 4];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 4]) << " ";
#endif
			}
			if (j + 5 < cache_width) {
				ME_cache.m_matrix[i][j + 5] = Host_cache.m_matrix[i][j + 5];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 5]) << " ";
#endif
			}
			if (j + 6 < cache_width) {
				ME_cache.m_matrix[i][j + 6] = Host_cache.m_matrix[i][j + 6];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 6]) << " ";
#endif
			}
			if (j + 7 < cache_width) {
				ME_cache.m_matrix[i][j + 7] = Host_cache.m_matrix[i][j + 7];
#ifdef DUMP_STIM
				p_MEcache_rtl << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i][j + 7]) << " ";
#endif
			}*/
		}
		p_MEcache_rtl << "\n";
	}
	p_MEcache_rtl << "\n";

	//START PREDICTION
	p_PeCost << "MB_Y: " << a / block_size << " MB_X: " << b / block_size << "\n";
	for (int i = 0; i <= cache_height - block_size; i++) {
		for (int j = 0; j < cache_width ; j += block_size) {
			for (int z = 0; z < 16; z++)//Make it config
				if (j + z + block_size <= cache_width) {
#ifdef JUAN_DEBUG
					for (int height=0; height < 16; height++) {
						for (int width=0; width < 16; width++) {
							p_C_cmodel << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_Cur_Block.m_matrix[height][width]) << " ";
						}
						p_C_cmodel << "\n";
					}
					p_C_cmodel << "\n";

					if (z == 0) {
						for (int height = 0; height < 16; height++) {
							for (int width = 0; width < 16; width++) {
								p_P_cmodel << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i+ height][j+ width]) << " ";
							}
							p_P_cmodel << "\n";
						}
						p_P_cmodel << "\n";

						for (int height = 0; height < 16; height++) {
							for (int width = 0; width < 16; width++) {
								if(j + width + 16 < cache_width)
									p_P_prime_cmodel << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(ME_cache.m_matrix[i + height][j + width +16]) << " ";
								else
									p_P_prime_cmodel << "0x" << std::setfill('0') << std::setw(2) << std::hex << int(255) << " ";

							}
							p_P_prime_cmodel << "\n";
						}
						p_P_prime_cmodel << "\n";

					}

#endif
					MV = search_vectors.pop();
					PE[z] = PFrame::ME(ME_Cur_Block, ME_cache, block_size, j + z, i);
#ifdef JUAN_DEBUG
					p_PeCost << "PE#" << z << " Cost=" << int(PE[z]) << " ";
#endif
					BestMV = (PE[z] < BestCost) ? MV : BestMV;
					BestCost = (PE[z] < BestCost) ? PE[z] : BestCost;
				}
		}
#ifdef JUAN_DEBUG
		p_PeCost << "\n";
#endif 
	}
#ifdef JUAN_DEBUG
	p_PeCost << "\n";
#endif				 
#ifdef DUMP_STIM
	p_MEmv_rtl << "MB_Y: " << a / block_size << " MB_X: " << b / block_size <<  " Cost: " << BestCost << " MV_Y: " << BestMV.first - a << " MV_X: "<< BestMV.second - b  << "\n";
#endif

	return  BestMV;
}


std::tuple<unsigned int, ByteMatrix, MV_T> PFrame::search_for_best_ref_hw(
	const COORD_T& cur_coord,
	const ByteMatrix& cur_block,
	const std::deque<Frame>& ref_frames,
	int r,
	unsigned int block_size,
	unsigned int qp,
	bool fast_me,
	const MV_T& last_mv)
{
	static int search_i, search_j, i_x, i_y;

	unsigned int min_cost = 0;
	int cache_startX = 0;
	int cache_startY = 0;
	ByteMatrix best_ref_block;
	ByteMatrix Host_cache;
	unsigned int cache_width;
	unsigned int cache_height;
	std::pair<int, int> mv_result;

	MV_T res_mv;

	for (unsigned int iref = 0; iref < ref_frames.size(); ++iref)
	{
		SearchQ search_vectors;
		//calculate offset
		cache_width = (r * 2) -1;
		cache_height = (r * 2) - 1;
		cache_startX = GetCachePos(cur_coord.second, r, ref_frames[iref].get_width(), cache_width, block_size);
		cache_startY = GetCachePos(cur_coord.first, r, ref_frames[iref].get_height(), cache_width, block_size);
		for (search_i = 0; search_i <= cache_height -block_size; ++search_i)
		{
			for (search_j = 0; search_j <= cache_width -block_size; ++search_j)//May need to change
			//for (search_j = 0; search_j < cache_width - block_size; ++search_j)//May need to change

			{
				search_vectors.push(std::make_pair(search_i+ cache_startY, search_j+ cache_startX));
			}
		}
		//Load cache
		COORD_T cache_coord(cache_startY, cache_startX);
		Host_cache = ref_frames[iref].get_y_block_at(cache_coord, cache_width);

#ifdef JUAN_DEBUG
/*		int a, b;
		p_cache_rtl << "MB_Y: "<< cur_coord.first/block_size  << " MB_X:  " << cur_coord.second/block_size << "\n";
		p_cache_cmodel << "MB_Y: " << cur_coord.first / block_size << " MB_X:  " << cur_coord.second / block_size << "\n";

		for (int i = 0; i < cache_height; i++) {
			for (int j = 0; j < cache_width; j++) {
				a = ME_cache.m_matrix[i][j];
				b = Host_cache.m_matrix[i][j];
				p_cache_rtl << a << " ";
				p_cache_cmodel << b << " ";
			}
			p_cache_rtl << "\n ";
			p_cache_cmodel  << "\n ";
		}
		p_cache_rtl << "\n";
		p_cache_cmodel << "\n ";*/
#endif
		//Load cache
		//START ME
		mv_result = StartME(Host_cache, cur_block, search_vectors, cache_width, cache_height,block_size, cur_coord.first, cur_coord.second);
		COORD_T search_coord(mv_result.first, mv_result.second);
		ByteMatrix ref_block = ref_frames[iref].get_y_block_at(search_coord, block_size);
		MV_T search_mv;
		search_mv.y = mv_result.first  - cur_coord.first;
		search_mv.x = mv_result.second - cur_coord.second;
		unsigned int mv_bytes = (search_mv == last_mv) ? 0 : sizeof(MV_T);
		unsigned int cost = ResidualBlock::estimate_rd_cost(cur_block, ref_block, qp, mv_bytes);
		best_ref_block = ref_block;
		res_mv = search_mv;
		res_mv.i = iref;
		min_cost = cost;
	}

	return std::make_tuple(min_cost, best_ref_block, res_mv);
}


PFrame::PFrame(const Frame& cur_frame, const std::deque<Frame>& ref_frames, unsigned int i, int r, unsigned int qp)
: m_block_size(i), m_frame_width(cur_frame.get_width()), m_frame_height(cur_frame.get_height())
{
	assert(ref_frames[0].get_width() == m_frame_width);
	assert(ref_frames[0].get_height() == m_frame_height);
	
	bool fast_me, vbs_enable, hw_enable;
	CFG_LOAD_OPT_DEFAULT("FastFME", fast_me, false);
	CFG_LOAD_OPT_DEFAULT("VBSEnable", vbs_enable, false);
	CFG_LOAD_OPT_DEFAULT("HwModeEnable", hw_enable, false);

	
	MV_T last_mv;
	
	for(auto& cur_block_t : cur_frame.get_y_block_vec(m_block_size))
	{
		COORD_T cur_coord = cur_block_t.first;
		const ByteMatrix& cur_block = cur_block_t.second;
		if(cur_coord.second == 0)
		{
			last_mv = MV_T(0,0,0);
		}
		
		unsigned int min_full_cost = 0, min_split_cost = std::numeric_limits<unsigned int>::max();
		ByteMatrix best_full_ref_block;
		MV_T full_res_mv;
		if(hw_enable)
			std::tie(min_full_cost, best_full_ref_block, full_res_mv) = PFrame::search_for_best_ref_hw(cur_coord, cur_block, ref_frames, r, m_block_size, qp, fast_me, last_mv);
		else
			std::tie(min_full_cost, best_full_ref_block, full_res_mv) = PFrame::search_for_best_ref ( cur_coord, cur_block, ref_frames, r, m_block_size, qp, fast_me, last_mv );
#ifdef JUAN_DEBUG
		p_mb_info << "MB_Y: " << cur_coord.first/m_block_size << " MB_X: " << cur_coord.second/m_block_size << " Cost: " << min_full_cost << " MV_Y: " << full_res_mv.y << " MV_X: " << full_res_mv.x << "\n";
#endif
		if(vbs_enable && m_block_size > 2 && m_block_size % 2 == 0)
		{
			ByteMatrix top_left_block = cur_frame.get_y_block_at(cur_coord, m_block_size / 2);
			
			unsigned int min_top_left_cost;
			ByteMatrix best_top_left_ref;
			MV_T top_left_res_mv;
			std::tie(min_top_left_cost, best_top_left_ref, top_left_res_mv) = PFrame::search_for_best_ref ( cur_coord, top_left_block, ref_frames, r, m_block_size/2, (qp>0)? qp-1: 0, fast_me, last_mv );
			last_mv = top_left_res_mv;
			
			cur_coord = calculate_next_coord(cur_coord, m_block_size, m_block_size/2, m_frame_width);
			
			ByteMatrix top_right_block = cur_frame.get_y_block_at(cur_coord, m_block_size / 2);
			unsigned int min_top_right_cost;
			ByteMatrix best_top_right_ref;
			MV_T top_right_res_mv;
			std::tie(min_top_right_cost, best_top_right_ref, top_right_res_mv) = PFrame::search_for_best_ref ( cur_coord, top_right_block, ref_frames, r, m_block_size/2, (qp>0)? qp-1: 0, fast_me, last_mv );
			last_mv = top_right_res_mv;
			
			cur_coord = calculate_next_coord(cur_coord, m_block_size, m_block_size/2, m_frame_width);
			
			ByteMatrix bot_left_block = cur_frame.get_y_block_at(cur_coord, m_block_size / 2);
			unsigned int min_bot_left_cost;
			ByteMatrix best_bot_left_ref;
			MV_T bot_left_res_mv;
			std::tie(min_bot_left_cost, best_bot_left_ref, bot_left_res_mv) = PFrame::search_for_best_ref ( cur_coord, bot_left_block, ref_frames, r, m_block_size/2, (qp>0)? qp-1: 0, fast_me, last_mv );
			last_mv = bot_left_res_mv;
			
			cur_coord = calculate_next_coord(cur_coord, m_block_size, m_block_size/2, m_frame_width);
			
			ByteMatrix bot_right_block = cur_frame.get_y_block_at(cur_coord, m_block_size / 2);
			unsigned int min_bot_right_cost;
			ByteMatrix best_bot_right_ref;
			MV_T bot_right_res_mv;
			std::tie(min_bot_right_cost, best_bot_right_ref, bot_right_res_mv) = PFrame::search_for_best_ref ( cur_coord, bot_right_block, ref_frames, r, m_block_size/2, (qp>0)? qp-1: 0, fast_me, last_mv );
			last_mv = bot_right_res_mv;
			
			min_split_cost = min_top_left_cost + min_top_right_cost + min_bot_left_cost + min_bot_right_cost;
			if(min_split_cost < min_full_cost)
			{
				ResidualBlock top_left_res_block(top_left_block, best_top_left_ref, (qp>0)? qp-1: 0, min_top_left_cost);
				assert(top_left_res_block.is_initialized());
				ResidualBlock top_right_res_block(top_right_block, best_top_right_ref, (qp>0)? qp-1: 0, min_top_right_cost);
				assert(top_right_res_block.is_initialized());
				ResidualBlock bot_left_res_block(bot_left_block, best_bot_left_ref, (qp>0)? qp-1: 0, min_bot_left_cost);
				assert(bot_left_res_block.is_initialized());
				ResidualBlock bot_right_res_block(bot_right_block, best_bot_right_ref, (qp>0)? qp-1: 0, min_bot_right_cost);
				assert(bot_right_res_block.is_initialized());
				
				m_mv_and_residuals.push_back(PF_REF_T(top_left_res_mv, top_left_res_block));
				m_mv_and_residuals.push_back(PF_REF_T(top_right_res_mv, top_right_res_block));
				m_mv_and_residuals.push_back(PF_REF_T(bot_left_res_mv, bot_left_res_block));
				m_mv_and_residuals.push_back(PF_REF_T(bot_right_res_mv, bot_right_res_block));
			}
		}
		
		if((vbs_enable && min_full_cost <= min_split_cost) || !vbs_enable)
		{
			ResidualBlock full_res_block(cur_block, best_full_ref_block, qp, min_full_cost);
			assert(full_res_block.is_initialized());
			
			m_mv_and_residuals.push_back(PF_REF_T(full_res_mv, full_res_block));
			last_mv = full_res_mv;
		}
	}
}
	
PFrame::PFrame(std::istream& mv_in, std::istream& res_in, unsigned int i, unsigned int frame_width, unsigned int frame_height, unsigned int qp)
: m_block_size(i), m_frame_width(frame_width), m_frame_height(frame_height)
{
	INT_VEC_T differential_mvs = RLE::read_and_irle_int_vec(mv_in);
	assert(differential_mvs.size() % 3 == 0);
	
	m_mv_and_residuals.resize(differential_mvs.size() / 3);
	MV_T last_mv(0, 0, 0);
	
	for(unsigned int i=0, imv=0; i < m_mv_and_residuals.size(); ++i, imv += 3)
	{
		ResidualBlock next_res(res_in, m_block_size, qp);
		MV_T cur_mv;
		cur_mv.x = last_mv.x - differential_mvs[imv];
		cur_mv.y = last_mv.y - differential_mvs[imv+1];
		cur_mv.i = last_mv.i - differential_mvs[imv+2];
		
		m_mv_and_residuals[i] = PF_REF_T(cur_mv, next_res);
		last_mv = cur_mv;
	}
}
	
unsigned int PFrame::write(std::ostream& mv_out, std::ostream& res_out)
{
	INT_VEC_T differential_mvs(m_mv_and_residuals.size() * 3);
	MV_T last_mv(0, 0, 0);
	unsigned int imv = 0;
	
	unsigned int bytes_written = 0;
	for(auto& mv_and_res_block: m_mv_and_residuals)
	{
		MV_T cur_mv =  mv_and_res_block.first;
		differential_mvs[imv] = last_mv.x - cur_mv.x;
		differential_mvs[imv+1] = last_mv.y - cur_mv.y;
		differential_mvs[imv+2] = last_mv.i - cur_mv.i;
		imv += 3;
		last_mv = cur_mv;
		
		bytes_written += mv_and_res_block.second.write(res_out);
	}	
	bytes_written += RLE::rle_and_write_int_vec(mv_out, differential_mvs);
	return bytes_written;
}
	
void PFrame::print(std::ostream& mv_out, std::ostream& res_out)
{
	assert(m_frame_width % m_block_size == 0);
	
	res_frame().print(res_out, true);
	
	unsigned int blocks_wide = m_frame_width/m_block_size;
	unsigned int iblock = 0;
	for(auto& mv_and_res_block: m_mv_and_residuals)
	{
		mv_out << "(dx=" << mv_and_res_block.first.x << ",dy=" << mv_and_res_block.first.y << ",if=" <<  mv_and_res_block.first.i << ")";
		iblock++;
		if (iblock == blocks_wide)
		{
			mv_out << "||";
			iblock = 0;
		}
	}
	mv_out << std::endl;
}

Frame PFrame::res_frame()
{
	std::vector<ResidualBlock> res_vec;
	for(auto& mv_and_res_block: m_mv_and_residuals)
	{	
		res_vec.push_back(mv_and_res_block.second);
	}
		
	return Frame(res_vec, m_block_size, m_frame_width, m_frame_height);
}

Frame PFrame::mv_frame(const std::deque<Frame>& ref_frames)
{
	BLOCKVEC_T ref_blocks(m_mv_and_residuals.size());

	COORD_T block_coord({0, 0});
	
	unsigned int ifr = 0;
	while(block_coord.first < m_frame_height)
	{
		assert(ifr < m_mv_and_residuals.size());
		
		MV_T ref_mv = m_mv_and_residuals[ifr].first;
		COORD_T ref_coord = PFrame::mv_to_coord(ref_mv, block_coord);
		unsigned int iref = (unsigned int)ref_mv.i;
		assert(iref < ref_frames.size());
	
		ByteMatrix ref_block = ref_frames[iref].get_y_block_at(ref_coord, m_mv_and_residuals[ifr].second.get_block_size());
		
		ref_blocks[ifr] = BLOCK_T(block_coord, ref_block);
		
		block_coord = calculate_next_coord(block_coord, m_block_size, m_mv_and_residuals[ifr].second.get_block_size(), m_frame_width);
		++ifr;
	}
	assert(block_coord.first == m_frame_height);
	return Frame(ref_blocks, m_block_size, m_frame_width, m_frame_height, get_block_colours());
}

INT_VEC_T PFrame::get_block_colours() const
{
	INT_VEC_T ret(m_mv_and_residuals.size());
	unsigned int i;
	for(i=0; i < m_mv_and_residuals.size(); ++i)
	{
		ret[i] = m_mv_and_residuals[i].first.i + 1;
	}
	return ret;
}
	

//**************************************************************************
// BEGIN IFRAME
//**************************************************************************

std::tuple<unsigned int, ByteMatrix, INTRA_MODE_T> choose_ref_block (
	const ByteMatrix& cur_block,
	const COORD_T& cur_coord,
	const ByteMatrix& recon_frame_above,
	const ByteMatrix& recon_row_left,
	const unsigned int full_block_size,
	const unsigned int qp,
	const INTRA_MODE_T& last_mode)
{
	unsigned int cur_block_size = cur_block.get_width();
	assert(cur_block_size == full_block_size || cur_block_size == full_block_size/2);
	
	unsigned int above_cost, left_cost;
	COORD_T above_coord, left_coord;
	
	above_coord.first = cur_coord.first - cur_coord.first % full_block_size - cur_block_size;
	above_coord.second = cur_coord.second;
	
	left_coord.first = cur_coord.first - recon_frame_above.get_height();
	left_coord.second = cur_coord.second - cur_coord.second % full_block_size - cur_block_size;
	
	ByteMatrix above_block(0x80, cur_block_size, cur_block_size);
	if(recon_frame_above.block_coord_is_legal(above_coord, cur_block_size))
	{
		above_block = recon_frame_above.get_block_at(above_coord, cur_block_size).generate_intra_mode_refblock(INTRA_MODE_ABOVE);
	}
	
	ByteMatrix left_block(0x80, cur_block_size, cur_block_size);
	if(recon_row_left.block_coord_is_legal(left_coord, cur_block_size))
	{
		left_block = recon_row_left.get_block_at(left_coord, cur_block_size).generate_intra_mode_refblock(INTRA_MODE_LEFT);
	}
	
	above_cost = ResidualBlock::estimate_rd_cost(cur_block, above_block, qp, (last_mode == INTRA_MODE_ABOVE)? 0 : sizeof(INTRA_MODE_T));
	left_cost = ResidualBlock::estimate_rd_cost(cur_block, left_block, qp, (last_mode == INTRA_MODE_LEFT)? 0 : sizeof(INTRA_MODE_T));
	
	if(above_cost < left_cost)
	{
		return std::make_tuple(above_cost, above_block, INTRA_MODE_ABOVE);
	}
	return std::make_tuple(left_cost, left_block, INTRA_MODE_LEFT);
}

IFrame::IFrame(const Frame& cur_frame, unsigned int i, unsigned int qp) :
m_block_size(i), m_frame_width(cur_frame.get_width()), m_frame_height(cur_frame.get_height())
{
	assert(m_frame_width % m_block_size == 0);
	assert(m_frame_height % m_block_size == 0);
	
	bool vbs_enable;
	CFG_LOAD_OPT_DEFAULT("VBSEnable", vbs_enable, false);
	
	ByteMatrix recon_frame, recon_row, recon_sub_block;
	
	COORD_T block_coord({0, 0}), ref_coord;
	INTRA_MODE_T last_mode = INTRA_MODE_LEFT;
	while(block_coord.first < m_frame_height)
	{
		ByteMatrix recon_row;
		std::deque<ByteMatrix> recon_sub_blocks;
		while(recon_row.get_width() < m_frame_width)
		{
			assert(block_coord.first < m_frame_height);
			
			unsigned int full_cost;
			ByteMatrix ref_block;
			INTRA_MODE_T mode;
			ByteMatrix cur_block = cur_frame.get_y_block_at(block_coord, m_block_size);
			std::tie(full_cost, ref_block, mode) = choose_ref_block(cur_block, block_coord, recon_frame, recon_row, m_block_size, qp, last_mode);
			
			unsigned int split_cost = std::numeric_limits<unsigned int>::max();
			if(vbs_enable && m_block_size > 2 && m_block_size % 2 == 0)
			{
				unsigned int sub_size = m_block_size/2;
				
				unsigned int tl_cost, tr_cost, bl_cost, br_cost;
				ByteMatrix tl_cur_block, tr_cur_block, bl_cur_block, br_cur_block;
				ByteMatrix tl_ref_block, tr_ref_block, bl_ref_block, br_ref_block;
				INTRA_MODE_T tl_mode, tr_mode, bl_mode, br_mode;
				COORD_T tl_coord, tr_coord, bl_coord, br_coord;
				
				tl_coord = block_coord;
				tl_cur_block = cur_frame.get_y_block_at(tl_coord, sub_size);
				std::tie(tl_cost, tl_ref_block, tl_mode) = choose_ref_block(tl_cur_block, tl_coord, recon_frame, recon_row, m_block_size, (qp>0)? qp-1: 0, last_mode);
				
				tr_coord = calculate_next_coord(tl_coord, m_block_size, sub_size, m_frame_width);
				tr_cur_block = cur_frame.get_y_block_at(tr_coord, sub_size);
				std::tie(tr_cost, tr_ref_block, tr_mode) = choose_ref_block(tr_cur_block, tr_coord, recon_frame, recon_row, m_block_size, (qp>0)? qp-1: 0, tl_mode);
				
				bl_coord = calculate_next_coord(tr_coord, m_block_size, sub_size, m_frame_width);
				bl_cur_block = cur_frame.get_y_block_at(bl_coord, sub_size);
				std::tie(bl_cost, bl_ref_block, bl_mode) = choose_ref_block(bl_cur_block, bl_coord, recon_frame, recon_row, m_block_size, (qp>0)? qp-1: 0, tr_mode);
				
				br_coord = calculate_next_coord(bl_coord, m_block_size, sub_size, m_frame_width);
				br_cur_block = cur_frame.get_y_block_at(br_coord, sub_size);
				std::tie(br_cost, br_ref_block, br_mode) = choose_ref_block(br_cur_block, br_coord, recon_frame, recon_row, m_block_size, (qp>0)? qp-1: 0, bl_mode);
				
				split_cost = tl_cost + tr_cost + bl_cost + br_cost;
				if(split_cost < full_cost)
				{
					ByteMatrix recon_block, bottom_row;
					
					{
						ResidualBlock tl_res(tl_cur_block, tl_ref_block, (qp>0)? qp-1: 0, tl_cost);
						assert(tl_res.is_initialized());
						ByteMatrix tl_recon_block = tl_res.reconstruct_from(tl_ref_block);
						m_ref_blocks.push_back(BLOCK_T(tl_coord, tl_ref_block));
						m_recon_blocks.push_back(BLOCK_T(tl_coord, tl_recon_block));
						m_modes_and_residuals.push_back(IF_REF_T(tl_mode, tl_res));
						recon_block = tl_recon_block;
					}
					
					{
						ResidualBlock tr_res(tr_cur_block, tr_ref_block, (qp>0)? qp-1: 0, tr_cost);
						assert(tr_res.is_initialized());
						ByteMatrix tr_recon_block = tr_res.reconstruct_from(tr_ref_block);
						m_ref_blocks.push_back(BLOCK_T(tr_coord, tr_ref_block));
						m_recon_blocks.push_back(BLOCK_T(tr_coord, tr_recon_block));
						m_modes_and_residuals.push_back(IF_REF_T(tr_mode, tr_res));
						recon_block.stitch_right(tr_recon_block);
					}
					
					{
						ResidualBlock bl_res(bl_cur_block, bl_ref_block, (qp>0)? qp-1: 0, bl_cost);
						assert(bl_res.is_initialized());
						ByteMatrix bl_recon_block = bl_res.reconstruct_from(bl_ref_block);
						m_ref_blocks.push_back(BLOCK_T(bl_coord, bl_ref_block));
						m_recon_blocks.push_back(BLOCK_T(bl_coord, bl_recon_block));
						m_modes_and_residuals.push_back(IF_REF_T(bl_mode, bl_res));
						bottom_row = bl_recon_block;
					}
					
					{
						ResidualBlock br_res(br_cur_block, br_ref_block, (qp>0)? qp-1: 0, br_cost);
						assert(br_res.is_initialized());
						ByteMatrix br_recon_block = br_res.reconstruct_from(br_ref_block);
						m_ref_blocks.push_back(BLOCK_T(br_coord, br_ref_block));
						m_recon_blocks.push_back(BLOCK_T(br_coord, br_recon_block));
						m_modes_and_residuals.push_back(IF_REF_T(br_mode, br_res));
						bottom_row.stitch_right(br_recon_block);
					}
					
					recon_block.stitch_below(bottom_row);
					recon_row.stitch_right(recon_block);
					last_mode = br_mode;
				}
			}
			
			if(!vbs_enable || full_cost <= split_cost)
			{
				ResidualBlock res(cur_block, ref_block, qp, full_cost);
				assert(res.is_initialized());
				ByteMatrix recon_block = res.reconstruct_from(ref_block);
				m_ref_blocks.push_back(BLOCK_T(block_coord, ref_block));
				m_recon_blocks.push_back(BLOCK_T(block_coord, recon_block));
				m_modes_and_residuals.push_back(IF_REF_T(mode, res));
				
				recon_row.stitch_right(recon_block);
				last_mode = mode;
			}
			
			block_coord = calculate_next_coord(block_coord, m_block_size, m_block_size, m_frame_width);
		}
		recon_frame.stitch_below(recon_row);
	}
}
	
IFrame::IFrame(std::istream& mode_in, std::istream& res_in, unsigned int i, unsigned int frame_width, unsigned int frame_height, unsigned int qp)
: m_block_size(i), m_frame_width(frame_width), m_frame_height(frame_height)
{
	assert(m_frame_width % m_block_size == 0);
	assert(m_frame_height % m_block_size == 0);
	
	INT_VEC_T differential_modes = RLE::read_and_irle_int_vec(mode_in);
	m_ref_blocks.resize(differential_modes.size());
	m_recon_blocks.resize(differential_modes.size());
	m_modes_and_residuals.resize(differential_modes.size());
	
	INTRA_MODE_T last_mode = INTRA_MODE_LEFT;
	unsigned int iblock = 0, res_block_size;
	ByteMatrix recon_frame;
	
	COORD_T block_coord({0, 0}), ref_coord;
	while(block_coord.first < m_frame_height)
	{
		ByteMatrix recon_row;
		std::deque<ByteMatrix> recon_sub_blocks;
		while(recon_row.get_width() < m_frame_width)
		{
			assert(iblock < differential_modes.size());
			assert(block_coord.first < m_frame_height);
				
			INTRA_MODE_T cur_mode = last_mode - differential_modes[iblock];
			last_mode = cur_mode;
			
			ResidualBlock res(res_in, m_block_size, qp);
			assert(res.is_initialized());
			res_block_size = res.get_block_size();
			
			ByteMatrix* ref_source = nullptr;
			ByteMatrix ref_block(0x80, res_block_size, res_block_size);
			if(cur_mode == INTRA_MODE_LEFT)
			{			
				ref_coord.first = block_coord.first - recon_frame.get_height();
				ref_coord.second = block_coord.second - res_block_size - block_coord.second % m_block_size;
				ref_source = &recon_row;
			}
			else if(cur_mode == INTRA_MODE_ABOVE)
			{
				ref_coord.first = block_coord.first - res_block_size - block_coord.first % m_block_size;
				ref_coord.second = block_coord.second;
				ref_source = &recon_frame;
			}
			
			if(ref_source->block_coord_is_legal(ref_coord, res_block_size))
			{
				ref_block = ref_source->get_block_at(ref_coord, res_block_size);
			}
			else
			{
				assert(block_coord.first == 0 || block_coord.second == 0 || block_coord.first == m_block_size/2 || block_coord.second == m_block_size/2);
			}
			
			ref_block = ref_block.generate_intra_mode_refblock(cur_mode);
			ByteMatrix recon_block = res.reconstruct_from(ref_block);
			if(res_block_size == m_block_size)
			{
				recon_row.stitch_right(recon_block);
			}
			else
			{
				assert(res_block_size == m_block_size/2);
				recon_sub_blocks.push_back(recon_block);
				if(recon_sub_blocks.size() == 4)
				{
					ByteMatrix top_row = recon_sub_blocks.front();
					recon_sub_blocks.pop_front();
					top_row.stitch_right(recon_sub_blocks.front());
					recon_sub_blocks.pop_front();
					
					ByteMatrix bottom_row = recon_sub_blocks.front();
					recon_sub_blocks.pop_front();
					bottom_row.stitch_right(recon_sub_blocks.front());
					recon_sub_blocks.pop_front();
					
					top_row.stitch_below(bottom_row);
					recon_row.stitch_right(top_row);
				}
			}
					
			m_ref_blocks[iblock] = BLOCK_T(block_coord, ref_block);
			m_recon_blocks[iblock] = BLOCK_T(block_coord, recon_block);
			m_modes_and_residuals[iblock] = IF_REF_T(cur_mode, res);
			
			block_coord = calculate_next_coord(block_coord, m_block_size, res_block_size, m_frame_width);
			++iblock;
		}
		recon_frame.stitch_below(recon_row);
	}
}
	
unsigned int IFrame::write(std::ostream& mode_out, std::ostream& res_out)
{
	INT_VEC_T differential_modes(m_modes_and_residuals.size());
	INTRA_MODE_T last_mode = INTRA_MODE_LEFT;
	unsigned int imd = 0;
	
	unsigned int bytes_written = 0;
	for(auto& mode_and_res_block: m_modes_and_residuals)
	{
		INTRA_MODE_T cur_mode = mode_and_res_block.first;
		differential_modes[imd] = last_mode - cur_mode;
		++imd;
		last_mode = cur_mode;
		
		bytes_written += mode_and_res_block.second.write(res_out);
	}	
	bytes_written += RLE::rle_and_write_int_vec(mode_out, differential_modes);
	return bytes_written;
}

void IFrame::print(std::ostream& mode_out, std::ostream& res_out)
{
	assert(m_frame_width % m_block_size == 0);
	
	res_frame().print(res_out, true);
	
	unsigned int blocks_wide = m_frame_width/m_block_size;
	unsigned int iblock = 0;
	for(auto& mode_and_res_block: m_modes_and_residuals)
	{
		mode_out << mode_and_res_block.first;
		iblock++;
		if (iblock == blocks_wide)
		{
			mode_out << "||";
			iblock = 0;
		}
		else
		{
			mode_out << " ";
		}
	}
	mode_out << std::endl;
}

Frame IFrame::res_frame() const
{
	std::vector<ResidualBlock> res_vec;
	for(auto& mode_and_res_block: m_modes_and_residuals)
	{	
		res_vec.push_back(mode_and_res_block.second);
	}
		
	return Frame(res_vec, m_block_size, m_frame_width, m_frame_height);
}

INT_VEC_T IFrame::get_block_colours() const
{
	INT_VEC_T ret(m_modes_and_residuals.size());
	unsigned int i;
	for(i=0; i < m_modes_and_residuals.size(); ++i)
	{
		if(m_modes_and_residuals[i].first == INTRA_MODE_LEFT)
		{
			ret[i] = 1;
		}
		else
		{
			ret[i] = 2;
		}
	}
	return ret;
}
	