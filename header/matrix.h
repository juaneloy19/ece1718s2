#include "util.h"
#include <iostream>

#ifndef _MATRIX_H
#define _MATRIX_H

const int INTRA_MODE_ABOVE = 0;
const int INTRA_MODE_LEFT = 1;

class ByteMatrix
{
public:
	ByteMatrix() : m_width(0), m_height(0) {};
	ByteMatrix(BYTEVEC_T vec, unsigned int width, unsigned int height);
	ByteMatrix(BYTE_T byte, unsigned int width, unsigned int height) : ByteMatrix(BYTEVEC_T(width*height, byte), width, height) {};
	
	void pad_width(unsigned int n, BYTE_T pad_val);
	void pad_height(unsigned int n, BYTE_T pad_val);
	
	unsigned int get_size()		const { return get_width()*get_height(); }
	unsigned int get_width() 	const { return m_width; }
	unsigned int get_height() 	const { return m_height; }
	
	BYTEVEC_T as_vec();
	
	std::vector< COORD_T > get_block_coords(unsigned int i) const;
	ByteMatrix get_block_at(COORD_T coord, unsigned int i) const;
	bool block_coord_is_legal(COORD_T coord, unsigned int i, bool expected_legal=false) const;
	
	void stitch_right(const ByteMatrix& rm);
	void stitch_below(const ByteMatrix& dm);
	void round_to_nearest_multiple(const BYTE_T& val);
	
	void print(std::ostream& out);
	
	unsigned int sum() const;
	BYTE_T average() const;
	unsigned int SAD_within_x(const BYTE_T& b, const BYTE_T& x) const;
	
	double covariance(const ByteMatrix& rhs) const;
	double variance() const { return covariance(*this); };

	double SSIM(const ByteMatrix& rhs) const;
	unsigned int SAD(const ByteMatrix& rhs) const;
	double MSE(const ByteMatrix& rhs) const;
	double PSNR(const ByteMatrix& rhs) const;

	const BYTEVEC_T& operator[](const unsigned int i) const { return m_matrix[i]; }
	ByteMatrix& operator+=(const ByteMatrix& rhs);
	friend ByteMatrix operator+(ByteMatrix lhs, const ByteMatrix& rhs) { lhs += rhs; return lhs; }
	ByteMatrix& operator-=(const ByteMatrix& rhs);
	friend ByteMatrix operator-(ByteMatrix lhs, const ByteMatrix& rhs) { lhs -= rhs; return lhs; }
	
	ByteMatrix generate_intra_mode_refblock(BYTE_T intra_mode);
	static ByteMatrix generate_border_block(BYTE_T core_byte, BYTE_T border_byte, unsigned int width, unsigned int height);
	//Juan
	std::vector< BYTEVEC_T > m_matrix;
private:
	unsigned int m_width;
	unsigned int m_height;

};

typedef std::pair<COORD_T, ByteMatrix> BLOCK_T;
typedef std::vector<BLOCK_T> BLOCKVEC_T;

#endif //_MATRIX_H