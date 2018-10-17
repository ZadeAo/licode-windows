#ifndef MD5_H
#define MD5_H

#include <string>
#include <fstream>

using std::string;
using std::ifstream;

/* MD5 declaration. */
class MD5 {


public:
	MD5();
	MD5(const void* input, size_t length);
	MD5(const string& str);
	MD5(ifstream& in);
	void update(const void* input, size_t length);
	void update(const string& str);
	void update(ifstream& in);
	const Byte* digest();
	string toString();
	void reset();

private:
	void update(const Byte* input, size_t length);
	void final();
	void transform(const Byte block[64]);
	void encode(const UInt32* input, Byte* output, size_t length);
	void decode(const Byte* input, UInt32* output, size_t length);
	string bytesToHexString(const Byte* input, size_t length);

	/* class uncopyable */
	MD5(const MD5&);
	MD5& operator=(const MD5&);

private:
	UInt32 _state[4];	/* state (ABCD) */
	UInt32 _count[2];	/* number of bits, modulo 2^64 (low-order word first) */
	Byte _buffer[64];	/* input buffer */
	Byte _digest[16];	/* message digest */
	bool _finished;		/* calculate finished ? */

	static const Byte PADDING[64];	/* padding for calculate */
	static const char HEX[16];
	enum { BUFFER_SIZE = 1024 };
};

#endif /*MD5_H*/
