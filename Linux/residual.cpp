//Juan
#include "matrix.h"
#include "residual.h"
#include "stdlib.h"
#include "math.h"
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <map>
#include <limits>
#include <sstream>

const COEF_T PI = atan(1.0) * 4.0;
	
const bool DISABLE_TRANSFORM = false;
const bool DISABLE_QUANTIZATION = false;
const bool ENABLE_COMPLEX_RDO_ESTIMATE = false;

std::pair<COEF_MATRIX_T, COEF_MATRIX_T> l_initialize_dct_coeffs(unsigned int N)
{
	static std::map< unsigned int, std::pair<COEF_MATRIX_T, COEF_MATRIX_T> > precomputed_values;
	
	auto p = precomputed_values.find(N);
	if (p == precomputed_values.end())
	{
		COEF_MATRIX_T C( N, std::vector<COEF_T>(N) );
		COEF_MATRIX_T Ct( N, std::vector<COEF_T>(N) );
		unsigned int i,j;
		
		for(j=0; j < N; ++j) {
			C[0][j] = 1.0 / sqrt( (COEF_T)N );
			Ct[j][0] = C[0][j];
		}
		
		for(i=1; i<N; ++i)	{
			for(j=0; j<N; j++) {
				C[i][j] = sqrt( 2.0 / (COEF_T)N) * cos ( PI * (2*j + 1) * i / (2.0 * (COEF_T)N) );
				Ct[j][i] = C[i][j];
			}
		}
		precomputed_values[N] = std::make_pair(C, Ct);
		p = precomputed_values.find(N);
	}
	
	assert(p != precomputed_values.end());
	return p->second;
}

COEF_MATRIX_T DCT::matrix_to_coefs(const ByteMatrix& matrix)
{
	assert(matrix.get_height() == matrix.get_width());
	unsigned int N = matrix.get_height();
	assert(N % 2 == 0);
	COEF_MATRIX_T ret( N, std::vector<COEF_T>(N) );
	unsigned int i, j, k;
	
	if (DISABLE_TRANSFORM)
	{
		for(i = 0; i < N; ++i){
			for(j = 0; j < N; ++j) {
				ret[i][j] = (COEF_T)matrix[i][j];
			}
		}
	}
	else
	{
		COEF_MATRIX_T temp( N, std::vector<COEF_T>(N) );
		COEF_MATRIX_T C, Ct;
		std::tie(C, Ct) = l_initialize_dct_coeffs(N);
		
		// transform the rows
		for(i = 0; i < N; ++i) {
			for(j = 0; j < N; ++j) {
				temp[i][j] = 0.0;
				for(k = 0; k < N; ++k)
				{
					temp[i][j] += ( (COEF_T)matrix[i][k] ) * Ct[k][j];
				}
			}
		}
		
		// transform the columns
		COEF_T temp_v;
		for(i=0; i < N; ++i) {
			for(j=0; j < N; ++j) {
				temp_v = 0.0;
				for(k=0; k < N; ++k) {
					temp_v += C[i][k] * temp[k][j];
				}
				ret[i][j] = temp_v;
			}
		}
	}
	return ret;
}

QCOEF_MATRIX_T DCT::quantize_coefs(const COEF_MATRIX_T& coefs, unsigned int qp)
{
	assert(coefs.size() == coefs[0].size());
	unsigned int N = coefs.size();
	assert(N % 2 == 0);

	QCOEF_MATRIX_T ret( N, std::vector<QCOEF_T>(N) );
	
	unsigned int i, j;
	if(DISABLE_QUANTIZATION) {
		for(i = 0; i < N; ++i) {
			for(j = 0; j < N; ++j) {
				ret[i][j] = coefs[i][j];
			}
		}
	} else {
		std::vector<COEF_T> quantization_values = { static_cast<COEF_T>(1 << qp), static_cast<COEF_T>(1 << (qp+1)), static_cast<COEF_T>(1 << (qp+2)) };	
		for(i=0; i<N; ++i) {
			for(j=0; j<N; ++j) {
				COEF_T q = quantization_values[0];
				if(i+j == N-1) {
					q = quantization_values[1];
				}
				else if (i+j > N-1) {
					q = quantization_values[2];
				}
				ret[i][j] = rint(coefs[i][j]/q);
			}
		}
	}
	return ret;
}

COEF_MATRIX_T DCT::rescale_coefs(const QCOEF_MATRIX_T& qcoefs, unsigned int qp)
{
	assert(qcoefs.size() > 0 && qcoefs.size() == qcoefs[0].size());
	unsigned int N = qcoefs.size();
	assert(N % 2 == 0);

	COEF_MATRIX_T ret( N, std::vector<COEF_T>(N) );
	unsigned int i, j;
	
	if(DISABLE_QUANTIZATION) {
		for(i = 0; i < N; ++i) {
			for(j = 0; j < N; ++j) {
				ret[i][j] = (COEF_T)qcoefs[i][j];
			}
		}
	} else {
		std::vector<COEF_T> quantization_values = { static_cast<COEF_T>(1 << qp), static_cast<COEF_T>(1 << (qp+1)), static_cast<COEF_T>(1 << (qp+2)) };
		for(i=0; i<N; ++i) {
			for(j=0; j<N; ++j) {
				COEF_T q = quantization_values[0];
				if(i+j == N-1) {
					q = quantization_values[1];
				}
				else if (i+j > N-1) {
					q = quantization_values[2];
				}
				ret[i][j] = q * (COEF_T)qcoefs[i][j];
			}
		}
	}
	return ret;
}

ByteMatrix DCT::coefs_to_matrix(const COEF_MATRIX_T& coefs)
{
	assert(coefs.size() > 0 && coefs.size() == coefs[0].size());
	unsigned int N = coefs.size();
	assert(N % 2 == 0);

	BYTEVEC_T values( N*N );
	
	unsigned int i, j, k;
	
	if (DISABLE_TRANSFORM)
	{
		for(i = 0; i < N; ++i){
			for(j = 0; j < N; ++j) {
				values[i*N + j] = (BYTE_T)coefs[i][j];
			}
		}
	}
	else
	{
		COEF_MATRIX_T temp( N, std::vector<COEF_T>(N) );
		COEF_MATRIX_T C, Ct;
		std::tie(C, Ct) = l_initialize_dct_coeffs(N);

		// transform the rows
		for(i = 0; i < N; ++i) {
			for(j = 0; j < N; ++j) {
				temp[i][j] = 0.0;
				for(k = 0; k < N; ++k)
				{
					temp[i][j] += coefs[i][k] * C[k][j];
				}
			}
		}
		
		// transform the columns
		COEF_T temp_v;
		for(i=0; i < N; ++i) {
			for(j=0; j < N; ++j) {
				temp_v = 0.0;
				for(k=0; k < N; ++k) {
					temp_v += Ct[i][k] * temp[k][j];
				}
				temp_v = std::max(temp_v, 0.);
				temp_v = std::min(temp_v, 255.);
				values[i*N + j] = static_cast<unsigned char>(rint(temp_v));	
			}
		}
	}
	
	return ByteMatrix(values, N, N);
}

void DCT::print_coefs(const COEF_MATRIX_T& coefs, std::ostream& out)
{
	for(auto& c_row : coefs)
	{
		for(auto& c : c_row)
		{
			out << std::setw(10) << c << " ";
		}
		out << std::endl;
	}
}

INT_VEC_T RLE::qcoef_matrix_to_int_vec(const QCOEF_MATRIX_T& qcoefs)
{
	unsigned int N = qcoefs.size();
	assert(N > 0 && N== qcoefs[0].size());
	INT_VEC_T ret(N* N);
	
	unsigned int diagonals = 2*N-1;
	unsigned int d, r, c, i=0;
	for(d=0; d<diagonals; ++d)
	{
		for(r=0, c=d; c>=0 && r < N; --c, ++r)
		{
			if(c < N)
			{
				ret[i] = qcoefs[r][c];
				++i;
			}
			
			if (c == 0) break;				
		}
	}
	assert(i = N*N);
	return ret;
}

QCOEF_MATRIX_T RLE::int_vec_to_qcoef_matrix(const INT_VEC_T& int_vec)
{
	unsigned int N = sqrt(int_vec.size());
	assert( int_vec.size() == N*N );
	
	QCOEF_MATRIX_T ret(N, std::vector<QCOEF_T>(N));
	
	unsigned int diagonals = 2*N-1;
	unsigned int d, r, c, i=0;
	for(d=0; d<diagonals; ++d)
	{
		for(r=0, c=d; c>=0 && r < N; --c, ++r)
		{
			if(c < N)
			{
				ret[r][c] = int_vec[i];
				++i;
			}
			
			if(c == 0) break;
		}
	}
	assert(i = N*N);
	return ret;
}

INT_VEC_T RLE::rle_int_vec(const INT_VEC_T& in)
{
	INT_VEC_T out;
	auto in_it = in.begin();
	
	unsigned int rsize_id;
	
	while(in_it != in.end())
	{
		rsize_id = out.size();
		out.push_back(0);
		
		if (*in_it == 0)
		{
			while(in_it != in.end() && *in_it == 0)
			{
				in_it++;
				out[rsize_id]++;
			}
		}
		else 
		{
			while (in_it != in.end() && *in_it != 0)
			{
				out.push_back( *in_it );
				in_it++;
				out[rsize_id]--;
			}
		}
	}
	return out;
}
	
INT_VEC_T RLE::irle_int_vec(const INT_VEC_T& in)
{
	INT_VEC_T out;
	
	auto in_it = in.begin();
	int i, num_ints;
	while(in_it != in.end())
	{
		assert( (*in_it) != 0 );
		if ((*in_it) < 0)
		{
			num_ints = (*in_it) * -1;
			in_it++;
			for(i=0; i < num_ints; ++i)
			{
				assert(in_it != in.end());
				out.push_back(*in_it);
				in_it++;
			}
		}
		else if ( (*in_it) > 0 )
		{
			num_ints = (*in_it);
			++in_it;
			for(i=0; i < num_ints; ++i)
			{
				out.push_back(0);
			}
		}
	}
	return out;
}

unsigned int RLE::rle_and_write_int_vec(std::ostream& out, const INT_VEC_T& int_vec)
{
	assert(int_vec.size() < std::numeric_limits<unsigned int>::max());
	assert(int_vec.size() > 0);
	
	INT_VEC_T vec_to_write = rle_int_vec(int_vec);	
	return GOLOMB::write_int_vec_to_stream(out, vec_to_write);
}

INT_VEC_T RLE::read_and_irle_int_vec(std::istream& in)
{
	INT_VEC_T read_vec = GOLOMB::read_int_vec_from_stream(in);	
	return irle_int_vec(read_vec);
}

ResidualBlock::ResidualBlock(const ByteMatrix& cur_block, const ByteMatrix& ref_block, unsigned int qp, unsigned int est_cost) : 
m_estimated_cost(est_cost), m_qp(qp), m_init(false)
{	
	assert(cur_block.get_width() != 0 && cur_block.get_width() == cur_block.get_height());
	assert(ref_block.get_width() != 0 && ref_block.get_width() == ref_block.get_height());
	assert(cur_block.get_width() == ref_block.get_height());
	m_block_size = ref_block.get_width();
	
	ByteMatrix block80(0x80, m_block_size, m_block_size);
	
	ByteMatrix spatial_residuals = cur_block - ref_block + block80;
	_dct_and_quantize(spatial_residuals);
	
	CFG_LOAD_OPT_DEFAULT("debug_res_est", m_debug_estimate, false);
	if(m_debug_estimate)
	{
		m_SAD = cur_block.SAD(ref_block);
	}
}
	
ResidualBlock::ResidualBlock(std::istream& in, unsigned int block_size, unsigned int qp) : 
m_estimated_cost(0), m_qp(qp), m_block_size(block_size), m_init(false)
{
	INT_VEC_T as_ints = RLE::read_and_irle_int_vec(in);
	m_residuals = RLE::int_vec_to_qcoef_matrix(as_ints);
	if(m_block_size == 2 * m_residuals.size() && m_block_size == 2* m_residuals[0].size())
	{
		m_block_size = m_residuals.size();
		if(m_qp > 0)
			--m_qp;
	}
	assert(m_residuals.size() == m_block_size && m_residuals[0].size() == m_block_size);
	m_init = true;
}

ByteMatrix ResidualBlock::reconstruct_from(const ByteMatrix& ref_block)
{
	assert(is_initialized());
	ByteMatrix spatial_residuals = as_y_block();
	
	assert(m_block_size == spatial_residuals.get_width());
	ByteMatrix block80(0x80, m_block_size, m_block_size);
	
	return ref_block + (spatial_residuals - block80);
}
	
unsigned int ResidualBlock::write(std::ostream& out, bool debug_enabled)
{
	assert(is_initialized());

	INT_VEC_T as_ints = RLE::qcoef_matrix_to_int_vec(m_residuals);
	m_bytes_written = RLE::rle_and_write_int_vec(out, as_ints);

	if(m_debug_estimate && debug_enabled)
	{
		DEBUG_CSV::push<unsigned int>({ m_estimated_cost, m_bytes_written, m_SAD });
	}
	
	return m_bytes_written;
}

void ResidualBlock::print(std::ostream& out)
{
	for(auto& row : m_residuals)
	{
		for(auto& r : row)
		{
			out << std::setw(10) << r;
		}
		out << std::endl;
	}
}

void ResidualBlock::_dct_and_quantize(const ByteMatrix& spatial_residuals)
{
	assert( !is_initialized() );
	COEF_MATRIX_T coefs = DCT::matrix_to_coefs(spatial_residuals);
	m_residuals = DCT::quantize_coefs(coefs, m_qp);
	m_init = true;
}

ByteMatrix ResidualBlock::_rescale_and_idct()
{
	assert(is_initialized());
	COEF_MATRIX_T rescaled_coefs = DCT::rescale_coefs(m_residuals, m_qp);
	return DCT::coefs_to_matrix(rescaled_coefs);
}

ByteMatrix ResidualBlock::as_y_block()
{
	return _rescale_and_idct();
}

unsigned int ResidualBlock::estimate_rd_cost(const ByteMatrix& cur, const ByteMatrix& ref, unsigned int qp, unsigned int additional_bytes)
{
	unsigned int SAD = cur.SAD(ref);
	
	// This is a very inner loop check, so use the following instead of string comparisons:
	// 0 = No RDO factor, aka SAD only
	// 1 = Simple RDO factor, based on the spatial-domain residuals
	// 2 = Complex RDO factor, based on the actual bytes written to disk
	unsigned int RDO_ESTIMATE, C1, C2;
	CFG_LOAD_OPT_DEFAULT("rdo_estimation", RDO_ESTIMATE, 0);
	CFG_LOAD_OPT_DEFAULT("rdo_estimation_c1", C1, 900); //882
	CFG_LOAD_OPT_DEFAULT("rdo_estimation_c2", C2, 900);
			
	unsigned int RDO_factor = 0;
	if(RDO_ESTIMATE == 2)
	{
		//Constructor may estimate a cost if debug_res_est is set; make sure it's not infinitely recursive!
		ResidualBlock r(cur, ref, qp, 0);
		std::stringstream ss;
		unsigned int bytes_written = r.write(ss, false);
		bytes_written += additional_bytes;
		
		RDO_factor = (int)( double(C2)*pow(2.0, (double(qp) - 12.)/3.)*(double)bytes_written );
	}
	else if (RDO_ESTIMATE == 1)
	{
		unsigned int bytes_written_est = (int)( double(SAD) * pow(2.0, 0 - double(qp)) );
		bytes_written_est += additional_bytes;
		
		RDO_factor = (int)( double(C1)*pow(2.0, (double(qp) - 12.)/3.)*(double)bytes_written_est );
	}
	
	return SAD + RDO_factor;
}