#include "golomb.h"
#include <map>
#include <cassert>
#include <algorithm>
#include <iomanip>

class GolombBytes
{
public:
	GolombBytes() : 							m_bytes(), m_istream(nullptr), m_byte_offset(0), m_bit_offset_mask(0x80) {};
	GolombBytes(std::istream* in) : 	m_bytes(), m_istream(in), m_byte_offset(0), m_bit_offset_mask(0x80) {};
	
	// Actually perform the Exponential-Golomb encoding and push it into the byte vector
	void push_int(const int& i);

	// Invert the exponential-golomb encoding for the next int in the byte vector
	int read_int();
	
	// Writing to a stream has to be byte-aligned, which means when reading we must
	// periodically round the offset up
	void round_offset_to_next_byte()
	{
		if(m_bit_offset_mask < 0x80)
		{
			m_byte_offset++;
			m_bit_offset_mask = 0x80;
		}
	}
	
	unsigned int write(std::ostream& out);
	
private:
	BYTEVEC_T m_bytes;
	std::istream* m_istream;
	unsigned int m_byte_offset;
	BYTE_T m_bit_offset_mask;
};

// Singleton for managing the GolombBytes on the decoder side
class GolombTracker
{
private:
	std::map< std::istream*, GolombBytes* > m_tracked_bytes;
	GolombTracker() {};
	~GolombTracker()
	{
		for(auto &ipair : m_tracked_bytes)
		{
			GolombBytes* bptr = ipair.second;
			delete bptr;
		}
	};
	
public:
	static GolombTracker& get_instance()
	{
		static GolombTracker instance;
		return instance;
	}
	
	GolombBytes* get_bytes_for_stream(std::istream& istream)
	{
		GolombBytes* ret = nullptr;
		auto it = m_tracked_bytes.find( &istream );
		if (it != m_tracked_bytes.end())
		{
			ret = it->second;
		}
		else
		{
			GolombBytes* new_bytes = new GolombBytes(&istream);
			m_tracked_bytes[&istream] = new_bytes;
			ret = new_bytes;
		}
		assert(ret != nullptr);
		return ret;
	}	
};

unsigned int GOLOMB::write_int_vec_to_stream(std::ostream& out, const INT_VEC_T& ivec)
{
	GolombBytes encoded_bytes;
	
	//First write the size
	encoded_bytes.push_int( (int)ivec.size() );
	
	//Now write the ints themselves
	for(auto& i : ivec)
	{
		encoded_bytes.push_int(i);
	}
	return encoded_bytes.write(out);
}

INT_VEC_T GOLOMB::read_int_vec_from_stream(std::istream& in)
{
	GolombBytes* encoded_bytes = GolombTracker::get_instance().get_bytes_for_stream(in);
	
	//First read the number of ints written
	unsigned int num_ints_to_read = (unsigned int)encoded_bytes->read_int();

	//Now read the ints themselves
	INT_VEC_T read_vec(num_ints_to_read);
	
	unsigned int i;
	for(i=0; i < num_ints_to_read; ++i)
	{
		read_vec[i] = encoded_bytes->read_int();
	}
	encoded_bytes->round_offset_to_next_byte();
	
	return read_vec;
}

BIT_T GOLOMB::get_bit(BYTEVEC_T& bytevec, const unsigned int byte_offset, const BYTE_T bit_mask, std::istream* bytestream)
{
	if(byte_offset == bytevec.size() && bytestream != nullptr)
	{
		assert(bit_mask == 0x80);
		char next;
		bytestream->get( next );
		assert(!bytestream->eof());
		
		bytevec.push_back((BYTE_T)next);
	}
	assert(byte_offset < bytevec.size());
	
	const BYTE_T& byte = bytevec[byte_offset];
	BIT_T masked = byte & bit_mask;
	if (masked != 0)
		return 1;
	return 0;
}

void GOLOMB::set_bit(BYTEVEC_T& bytevec, const unsigned int byte_offset, const BYTE_T bit_mask,  BIT_T bit)
{
	if(byte_offset == bytevec.size())
	{
		assert(bit_mask == 0x80);
		bytevec.push_back(0x00);
	}
	assert(byte_offset < bytevec.size());
	
	if(bit == 1)
	{
		bytevec[byte_offset] |= bit_mask;
	}
	else
	{
		bytevec[byte_offset] &= ~bit_mask;
	}
}

void GOLOMB::incr_bit(unsigned int& byte_offset, BYTE_T& bit_mask)
{
	bit_mask = bit_mask >> 1;
	if(bit_mask == 0)
	{
		byte_offset++;
		bit_mask = 0x80;
	}
}

void GOLOMB::decr_bit(unsigned int& byte_offset, BYTE_T& bit_mask)
{
	bit_mask = bit_mask << 1;
	if(bit_mask == 0)
	{
		byte_offset--;
		bit_mask = 0x01;
	}
}

void GOLOMB::encode_unsigned_int_to_bytes(const unsigned int i, BYTEVEC_T& bytes, unsigned int& byte_offset, BYTE_T& bit_mask)
{
	// Transform 0 or less using extension to negative numbers; by the time it's here we should only be encoding positive numbers
	assert(i > 0);
	BYTEVEC_T int_bytes;
	const BYTE_T* raw_int_bytes = reinterpret_cast<const BYTE_T*>(&i);
	int_bytes.insert(int_bytes.begin(), raw_int_bytes, raw_int_bytes + sizeof(unsigned int));	
	
	//Tested and developed on a machine that's little-endian; if run on a big-endian processor this result in bigger bitstreams than desired
	std::reverse(int_bytes.begin(), int_bytes.end());
	
	unsigned int int_byte_offset = 0;
	BYTE_T int_bit_mask = 0x80;
	
	// Find the first bit in the int that's set
	while(get_bit(int_bytes, int_byte_offset, int_bit_mask) == 0)
	{
		incr_bit(int_byte_offset, int_bit_mask);
	}
	
	// Now, for each bit remaining (less 1), push a 0 to the out stream...
	unsigned int temp_offset = int_byte_offset;
	BYTE_T temp_bit_mask = int_bit_mask;
	incr_bit(temp_offset, temp_bit_mask);
	
	//unsigned int leading_zeroes = 0;
	while(temp_offset < int_bytes.size())
	{
		set_bit(bytes, byte_offset, bit_mask, 0);
		//leading_zeroes++;
		incr_bit(temp_offset, temp_bit_mask);
		incr_bit(byte_offset, bit_mask);
	}
	
	
	//... and then push the actual values
	//unsigned int data_bits = 0;
	while(int_byte_offset < int_bytes.size())
	{
		BIT_T bit = get_bit(int_bytes, int_byte_offset, int_bit_mask);
		//data_bits++;
		set_bit(bytes, byte_offset, bit_mask, bit);
		
		incr_bit(int_byte_offset, int_bit_mask);
		incr_bit(byte_offset, bit_mask);
	}
	//std::cout << "[" << leading_zeroes << "-0's, " << data_bits << "-b's]";
}

unsigned int GOLOMB::decode_unsigned_int_from_bytes(BYTEVEC_T& bytes, unsigned int& byte_offset, BYTE_T& bit_mask, std::istream* bytestream)
{
	BYTEVEC_T int_bytes(sizeof(unsigned int), 0x00);
	
	unsigned int int_byte_offset = int_bytes.size() - 1;
	BYTE_T int_bit_mask = 0x01;
	
	//Index from the right in the int bit vector for each leading 0 in the bytes
	//unsigned int leading_zeroes = 0;
	while(get_bit(bytes, byte_offset, bit_mask, bytestream) == 0)
	{
		//leading_zeroes++;
		incr_bit(byte_offset, bit_mask);
		decr_bit(int_byte_offset, int_bit_mask);
	}
	
	//Now go back through the int and set the bits to match
	//unsigned int data_bits = 0;
	while(int_byte_offset < int_bytes.size())
	{
		BIT_T bit = get_bit(bytes, byte_offset, bit_mask, bytestream);
		//data_bits++;
		set_bit(int_bytes, int_byte_offset, int_bit_mask, bit);
		
		incr_bit(int_byte_offset, int_bit_mask);
		incr_bit(byte_offset, bit_mask);
	}
	//std::cout << "[" << leading_zeroes << "-0's, " << data_bits << "-b's]";
	
	//Tested and developed on a machine that's little-endian; if run on a big-endian processor this result in bigger bitstreams than desired
	std::reverse(int_bytes.begin(), int_bytes.end());
	
	unsigned int decoded = *reinterpret_cast<unsigned int*>(&int_bytes[0]);
	return decoded;
}
	
unsigned int GolombBytes::write(std::ostream& out)
{
	unsigned int num_bytes = m_bytes.size();
	out.write(reinterpret_cast<const char*>(&m_bytes[0]), num_bytes*sizeof(BYTE_T));
	return num_bytes*sizeof(BYTE_T);
}

void GolombBytes::push_int(const int& i)
{
	unsigned int num_to_encode = 0;
	if( i <= 0 )
	{
		num_to_encode = (unsigned int)(-2 * i + 1);
	}
	else
	{
		num_to_encode = 2 * i;
	}
	assert(num_to_encode > 0);
	
	//TODO: Implement Exponential-Golomb encoding
	GOLOMB::encode_unsigned_int_to_bytes(num_to_encode, m_bytes, m_byte_offset, m_bit_offset_mask);
	
	/*const BYTE_T* bytes = reinterpret_cast<const BYTE_T*>(&num_to_encode);
	m_bytes.insert(m_bytes.begin() + m_byte_offset, bytes, bytes + sizeof(int));*/
	
	// read_int will increment the offsets accordingly and ensure the data can be safely extracted
	/*int read = read_int();
	if (i != read)
	{
		std::cout << i << " != " << read << std::endl;
	}
	assert(i == read);*/
}

int GolombBytes::read_int()
{	
	//TODO: Implement Exponential-Golomb decoding
	/*assert(m_bit_offset_mask == 0x80);
	assert(m_byte_offset + sizeof(int) <= m_bytes.size() || (m_istream != nullptr && m_byte_offset == m_bytes.size()));

	if(m_byte_offset == m_bytes.size() && m_istream != nullptr)
	{
		// We need to load more bytes from the stream! For now, we know we need to load sizeof(int) more
		m_bytes.resize(m_bytes.size() + sizeof(int));
		m_istream->read(reinterpret_cast<char*>(&m_bytes[m_byte_offset]), sizeof(int));
		assert(m_istream);
	}
	
	unsigned int decoded = *reinterpret_cast<unsigned int*>(&m_bytes[m_byte_offset]);
	m_byte_offset += sizeof(int);*/
	unsigned int decoded = GOLOMB::decode_unsigned_int_from_bytes(m_bytes, m_byte_offset, m_bit_offset_mask, m_istream);
	
	int ret = 0;
	if(decoded % 2 == 0)
	{
		ret = decoded / 2;
	}
	else
	{
		ret = (int)(decoded - 1) / -2;
	}
	
	return ret;
}