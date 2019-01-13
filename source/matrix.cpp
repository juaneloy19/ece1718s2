#include <cmath>
#include <cassert>
#include <iomanip>
#include <limits>
#include "matrix.h"

ByteMatrix::ByteMatrix(std::vector<BYTE_T> vec, unsigned int width, unsigned int height)
: m_width(width), m_height(height)
{
	if (vec.size() != m_width * m_height)
	{
		std::cout << "ERROR: constructing a " << width << "x" << height << " matrix out of " << vec.size() << " bytes" << std::endl;
	}
	assert(vec.size() == m_width * m_height);
	
	for (auto it = vec.begin(); it != vec.end(); it += m_width)
	{
		m_matrix.push_back(BYTEVEC_T(it, it+m_width));
	}
	assert(m_matrix.size() == m_height);
}

std::vector<BYTE_T> ByteMatrix::as_vec()
{
	std::vector<BYTE_T> ret;
	for (auto& r: m_matrix)
	{
		ret.insert(ret.end(), r.begin(), r.end());
	}
	return ret;
}

void ByteMatrix::pad_width(unsigned int n, BYTE_T pad_val)
{
	for (auto& r: m_matrix)
	{
		r.insert(r.end(), n, pad_val);
	}
	m_width += n;
}

void ByteMatrix::pad_height(unsigned int n, BYTE_T pad_val)
{
	for(unsigned int i = 0; i < n; ++i)
	{
		m_matrix.push_back(BYTEVEC_T(m_width, pad_val));
	}
	m_height += n;
}

std::vector<COORD_T> ByteMatrix::get_block_coords(unsigned int i) const
{
	assert(m_width % i == 0 && m_height % i == 0);
	
	// Characterize each block by the upper-left hand coordinate
	std::vector<COORD_T> ret;
	for (unsigned int row = 0; row < m_height; row += i)
	{	
		for (unsigned int col = 0; col < m_width; col += i)
		{
			ret.push_back(COORD_T(row, col));
		}
	}
	
	assert(ret.size() == m_height * m_width / (i*i));
	return ret;
}

ByteMatrix ByteMatrix::get_block_at(COORD_T coord, unsigned int i) const
{
	assert(block_coord_is_legal(coord, i, true));
	
	std::vector<BYTE_T> vec;
	for(unsigned int row = coord.first; row < coord.first + i; row++)
	{
		for(unsigned int col = coord.second; col < coord.second + i; col++)
		{
			vec.push_back(m_matrix[row][col]);
		}
	}
	return ByteMatrix(vec, i, i);
}

bool ByteMatrix::block_coord_is_legal(COORD_T coord, unsigned int i, bool expected_legal) const
{
	bool legal = ( coord.first < m_height && coord.second < m_width && coord.first + i <= m_height && coord.second + i <= m_width);
	if (!legal && expected_legal)
	{
		std::cout << "ERROR: Asked for block of size " << i << " at location (" << coord.first << "," << coord.second << ") where m_height=" << m_height << " and m_width=" << m_width << std::endl;
		std::cout << std::flush;
	}
	return legal;
}

void ByteMatrix::print(std::ostream& out)
{
	for(auto& r : m_matrix)
	{
		for(auto& elem : r)
		{
			out << std::setw(3) << static_cast<unsigned>(elem) << " ";
		}
		out << std::endl;
	}
}

void ByteMatrix::stitch_right(const ByteMatrix& rm)
{
	if (m_height == 0 && m_width == 0)
	{
		m_matrix = rm.m_matrix;
		m_width = rm.get_width();
		m_height = rm.get_height();
	}
	else
	{
		assert(rm.m_height == m_height);
		for(unsigned int irow = 0; irow < m_height; irow++)
		{
			m_matrix[irow].insert(m_matrix[irow].end(), rm.m_matrix[irow].begin(), rm.m_matrix[irow].end());
		}
		m_width += rm.get_width();
	}
}

void ByteMatrix::stitch_below(const ByteMatrix& dm)
{
	if (m_height == 0 && m_width == 0)
	{
		m_matrix = dm.m_matrix;
		m_width = dm.get_width();
		m_height = dm.get_height();
	}
	else
	{
		assert(dm.m_width == m_width);
		m_matrix.insert(m_matrix.end(), dm.m_matrix.begin(), dm.m_matrix.end());
		m_height += dm.get_height();
	}
}

void ByteMatrix::round_to_nearest_multiple(const BYTE_T& val)
{
	for(unsigned int i = 0; i < m_height; ++i)
	{
		for(unsigned int j = 0; j < m_width; ++j)
		{
			short result = short(m_matrix[i][j]) + short(val)/2;
			result -= result % short(val);
			m_matrix[i][j] = BYTE_T(result);
		}
	}
}
	
ByteMatrix& ByteMatrix::operator+=(const ByteMatrix& rhs)
{
	assert(rhs.m_height == m_height);
	assert(rhs.m_width == m_width);
	for(unsigned int irow = 0; irow < m_height; irow++)
	{
		for(unsigned int icol = 0; icol < m_width; icol++)
		{
			m_matrix[irow][icol] += rhs.m_matrix[irow][icol];
		}
	}
	return (*this);
}

ByteMatrix& ByteMatrix::operator-=(const ByteMatrix& rhs)
{
	assert(rhs.m_height == m_height);
	assert(rhs.m_width == m_width);
	for(unsigned int irow = 0; irow < m_height; irow++)
	{
		for(unsigned int icol = 0; icol < m_width; icol++)
		{
			m_matrix[irow][icol] -= rhs.m_matrix[irow][icol];
		}
	}
	return (*this);
}

unsigned int ByteMatrix::sum() const
{
	unsigned int total;
	total = 0;
	for(auto& row : m_matrix)
	{
		for(auto& elem : row)
		{
			total += static_cast<unsigned int>(elem);
		}
	}
	return total;
}

BYTE_T ByteMatrix::average() const
{
	return static_cast<BYTE_T>( sum() / (m_width*m_height) );
}

unsigned int ByteMatrix::SAD_within_x(const BYTE_T& b, const BYTE_T& x) const
{
	assert(x != 0x00);

	int max = int(b)+int(x);
	int min = int(b)-int(x);
	
	unsigned int c = 0;
	for(auto& r : m_matrix)
	{
		for(auto& e : r)
		{
			if(int(e) <= max && int(e) >= min)
			{
				c += abs(int(e) - int(b));
			}
			else
			{
				c += int(x);
			}
		}
	}
	return c;
}

double ByteMatrix::covariance(const ByteMatrix& rhs) const
{
	assert(m_height == rhs.m_height);
	assert(m_width == rhs.m_width);
	assert(m_height > 0 && m_width > 0);
	
	double my_average = double(average());
	double rhs_average = double(rhs.average());
	double ret = 0;
	for(unsigned int i = 0; i < m_height; ++i)
	{
		for(unsigned int j = 0; j < m_width; ++j)
		{
			ret += (double(m_matrix[i][j]) - my_average)*(double(rhs.m_matrix[i][j]) - rhs_average);
		}
	}
	return ret / double(m_height*m_width);
}

double ByteMatrix::SSIM(const ByteMatrix& rhs) const
{
	assert(m_height == rhs.m_height);
	assert(m_width == rhs.m_width);
	
	double my_average = double(average());
	double my_variance = double(variance());
	
	double rhs_average = double(rhs.average());
	double rhs_variance = double(rhs.variance());
	
	double covar = double(covariance(rhs));
	
	double L = pow(2, sizeof(BYTE_T)) - 1;
	double k1 = 0.01;
	double k2 = 0.03;
	double c1 = (k1*L)*(k1*L);
	double c2 = (k2*L)*(k2*L);
	
	return ( (2*my_average*rhs_average + c1)*(2*covar + c2))/( (my_average*my_average + rhs_average*rhs_average + c1)*(my_variance + rhs_variance + c2) );
}

double ByteMatrix::MSE(const ByteMatrix& rhs) const
{
	assert(m_height == rhs.m_height);
	assert(m_width == rhs.m_width);
	
	double ret = 0;
	for(unsigned int i = 0; i < m_height; ++i)
	{
		for(unsigned int j = 0; j < m_width; ++j)
		{
			double diff = double(m_matrix[i][j]) - double(rhs.m_matrix[i][j]);
			ret += diff*diff;
		}
	}
	return ret / double(m_height*m_width);
}

double ByteMatrix::PSNR(const ByteMatrix& rhs) const
{
	assert(m_height == rhs.m_height);
	assert(m_width == rhs.m_width);
	
	double mse = MSE(rhs);
	double max = std::numeric_limits<BYTE_T>::max();
	return 20.0*log10(max) - 10.0*log10(mse);
}

unsigned int ByteMatrix::SAD(const ByteMatrix& rhs) const
{
	assert(m_height == rhs.m_height);
	assert(m_width == rhs.m_width);
	
	unsigned int ret = 0;
	for(unsigned int i = 0; i < m_height; ++i)
	{
		for(unsigned int j = 0; j < m_width; ++j)
		{
			ret += abs( int(m_matrix[i][j]) - int(rhs.m_matrix[i][j]) );
		}
	}
	return ret;
}

ByteMatrix ByteMatrix::generate_intra_mode_refblock(BYTE_T intra_mode)
{
	assert(intra_mode == INTRA_MODE_ABOVE || intra_mode == INTRA_MODE_LEFT);
	ByteMatrix ret(0x00, m_width, m_height);
	unsigned int i;
	for(i=0; i < m_height; ++i)
	{
		if(intra_mode == INTRA_MODE_ABOVE)
		{
			ret.m_matrix[i] = m_matrix[m_height-1];
		}
		else
		{
			ret.m_matrix[i] = BYTEVEC_T(m_width, m_matrix[i].back());
		}
	}
	return ret;
}

ByteMatrix ByteMatrix::generate_border_block(BYTE_T core_byte, BYTE_T border_byte, unsigned int width, unsigned int height)
{
	ByteMatrix m(core_byte, width, height);
	for(auto& r : m.m_matrix)
	{
		auto it = r.end() -1;
		(*it) = border_byte;
	}
	m.m_matrix.back() = BYTEVEC_T(width, border_byte);
	return m;
}