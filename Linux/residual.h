#include "matrix.h"
#include "golomb.h"
#include <vector>

#ifndef _RESIDUAL_H
#define _RESIDUAL_H

typedef double COEF_T;
typedef int QCOEF_T;
typedef std::vector< std::vector< COEF_T > > COEF_MATRIX_T;
typedef std::vector< std::vector< QCOEF_T > > QCOEF_MATRIX_T;

namespace DCT
{	
	COEF_MATRIX_T matrix_to_coefs(const ByteMatrix& matrix);
	
	QCOEF_MATRIX_T quantize_coefs(const COEF_MATRIX_T& coefs, unsigned int qp);
	COEF_MATRIX_T rescale_coefs(const QCOEF_MATRIX_T& coefs, unsigned int qp);
	
	ByteMatrix coefs_to_matrix(const COEF_MATRIX_T& coefs);
	
	void print_coefs(const COEF_MATRIX_T& coefs, std::ostream& out);
}

namespace RLE
{
	//Helper function to convert a matrix of quantized coefficients to/from an int vec
	INT_VEC_T qcoef_matrix_to_int_vec(const QCOEF_MATRIX_T& qcoefs);
	QCOEF_MATRIX_T int_vec_to_qcoef_matrix(const INT_VEC_T& int_vec);
	
	//Actually perform the Run-Length Encoding of a vector of integers
	INT_VEC_T rle_int_vec(const INT_VEC_T& in);
	INT_VEC_T irle_int_vec(const INT_VEC_T& in);
	
	//Helper function to write/read an int vec to/from a stream
	unsigned int rle_and_write_int_vec(std::ostream& out, const INT_VEC_T& int_vec);
	INT_VEC_T read_and_irle_int_vec(std::istream& in);
}

class ResidualBlock
{
public:
	ResidualBlock() : m_init(false) {};
	
	// Encoder-side creation of residuals based on a prediction
	ResidualBlock(const ByteMatrix& cur_block, const ByteMatrix& ref_block, unsigned int qp, unsigned int est_cost);
	
	// Decoder-side creation of residuals from a byte stream
	ResidualBlock(std::istream& in, unsigned int block_size, unsigned int qp);
	
	// Generate a reconstructed block from a reference block
	ByteMatrix reconstruct_from(const ByteMatrix& ref_block);
	
	// Write to a byte stream
	unsigned int write(std::ostream& out) { return write(out, true); }
	
	// Print to a human-readable stream
	void print(std::ostream& out);
	
	// Check if we've initialized the block (copy constructor or one of the data constructors)
	bool is_initialized() { return m_init; };
	
	// Return a ByteMatrix with values representing the residuals in the spatial domain
	ByteMatrix as_y_block();
	
	// Estimate the RD-Cost of creating a ResidualBlock from two frames
	static unsigned int estimate_rd_cost(const ByteMatrix& cur, const ByteMatrix& ref, unsigned int qp, unsigned int additional_bytes);
	
	unsigned int get_block_size() { return m_block_size; }
	
private:
	unsigned int write(std::ostream& out, bool debug_enabled);
	
	// Used for debugging purposes (debug_res_est=on)
	bool m_debug_estimate;
	unsigned int m_estimated_cost;
	unsigned int m_SAD;

	void _dct_and_quantize(const ByteMatrix& spatial_residuals);
	ByteMatrix _rescale_and_idct();
	
	QCOEF_MATRIX_T m_residuals;
	unsigned int m_qp;
	unsigned int m_bytes_written;
	unsigned int m_block_size;

	bool m_init;
};

#endif //_RESIDUAL_H