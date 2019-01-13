#include "util.h"
#include <iostream>

#ifndef _GOLOMB_H
#define _GOLOMB_H

typedef unsigned char BIT_T;

namespace GOLOMB
{
	unsigned int write_int_vec_to_stream(std::ostream& out, const INT_VEC_T& ivec);
	INT_VEC_T read_int_vec_from_stream(std::istream& in);
	
	BIT_T get_bit(BYTEVEC_T& byte_vec, const unsigned int byte_offset, const BYTE_T bit_mask, std::istream* bytestream = nullptr);
	void set_bit(BYTEVEC_T& byte_vec, const unsigned int byte_offset, const BYTE_T bit_mask,  BIT_T bit);
	
	void incr_bit(unsigned int& byte_offset, BYTE_T& bit_mask);
	void decr_bit(unsigned int& byte_offset, BYTE_T& bit_mask);
	
	void encode_unsigned_int_to_bytes(const unsigned int i, BYTEVEC_T& bytes, unsigned int& byte_offset, BYTE_T& bit_offset_mask);
	unsigned int decode_unsigned_int_from_bytes(BYTEVEC_T& bytes, unsigned int& byte_offset, BYTE_T& bit_mask, std::istream* bytestream = nullptr);
}

#endif // _GOLOMB_H