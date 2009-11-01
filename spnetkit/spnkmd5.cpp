/*
 * Based on a public domain implementation:
 *
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * SP_NKMd5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
#include <string.h>    /*! for memcpy() */
#include "spnkmd5.hpp"

int SP_NKMd5 :: md5sum( unsigned char const * buf, int buflen, char * hex, int len )
{
  if( len < 33 ) return -1;

  SP_NKMd5 md5;

  md5.update( buf, buflen );
  md5.end( hex );

  return 0;
}

#ifdef WORDS_BIGENDIAN
void
byteSwap(unsigned int *buf, unsigned int words)
{
  unsigned char *p = (unsigned char *)buf;

  do {
    *buf++ = (unsigned long)((unsigned int)p[3] << 8 | p[2]) << 16 |
      ((unsigned int)p[1] << 8 | p[0]);
    p += 4;
  } while (--words);
}
#else
/*! On the little endian architectures does nothing. */
#define byteSwap(buf,words)
#endif

/*!
 *Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 *initialization constants.
 */
void SP_NKMd5::init()
{
  buf[0] = 0x67452301;
  buf[1] = 0xefcdab89;
  buf[2] = 0x98badcfe;
  buf[3] = 0x10325476;

  bytes[0] = 0;
  bytes[1] = 0;
}

/*!
 *Update context to reflect the concatenation of another buffer full
 *of bytes.
 */
void SP_NKMd5::update(unsigned char const *buf, unsigned long len)
{
  unsigned int t;

  /* Update byte count */

  t = bytes[0];
  if ((bytes[0] = t + len) < t)
    bytes[1]++;  /* Carry from low to high */

  t = 64 - (t & 0x3f);  /* Space available in in (at least 1) */
  if (t > len) 
  {
    memcpy((unsigned char *)in + 64 - t, buf, len);
    return;
  }
  /* First chunk is an odd size */
  memcpy((unsigned char *)in + 64 - t, buf, t);
  byteSwap(in, 16);
  transform( this->buf, in);
  buf += t;
  len -= t;

  /* Process data in 64-byte chunks */
  while (len >= 64) 
  {
    memcpy(in, buf, 64);
    byteSwap(in, 16);
    transform(this->buf, in);
    buf += 64;
    len -= 64;
  }

  /* Handle any remaining bytes of data. */
  memcpy(in, buf, len);
}

/*!
 *Final wrapup - pad to 64-byte boundary with the bit pattern 
 *1 0* (64-bit count of bits processed, MSB-first)
 */
void SP_NKMd5::final(unsigned char digest[16])
{
  long count = bytes[0] & 0x3f;  /* Number of bytes mod 64. */
  unsigned char *p = (unsigned char *)in + count;
  unsigned int i;
  /* Set the first char of padding to 0x80.  There is always room. */
  *p++ = 0x80;

  /* Bytes of padding needed to make 56 bytes (-8..55) */
  count = 56 - 1 - count;

  if (count < 0) {  /* Padding forces an extra block */
    memset(p, 0, count + 8);
    byteSwap(in, 16);
    transform( buf, in);
    p = (unsigned char *)in;
    count = 56;
  }
  memset(p, 0, count);
  byteSwap(in, 14);

  /* Append length in bits and transform */
  in[14] = bytes[0] << 3;
  in[15] = bytes[1] << 3 | bytes[0] >> 29;
  transform(buf, in);

  byteSwap(buf, 4);

  memcpy(digest, buf, 16);

  for(i = 0; i < 4; i++)
    buf[i] = 0;

  for(i = 0; i < 2; i++)
    bytes[i] = 0;

  for(i = 0; i < 16; i++) 
    in[i] = 0;

}

#ifndef ASM_MD5

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
   (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/*!
 *The core of the MD5 algorithm, this alters an existing MD5 hash to
 *reflect the addition of 16 longwords of new data.  update blocks
 *the data and converts bytes longo longwords for this routine.
 */
void SP_NKMd5::transform(unsigned int buf[4], unsigned int const in[16])
{
  register unsigned int a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

/*!
 *Initialize the object via a constructor.
 */
SP_NKMd5::SP_NKMd5()
{
  init();
}

/*!
 *Destroy the object.
 */
SP_NKMd5::~SP_NKMd5()
{}

/*!
 *Write the final hash to the buffer.
 */
char* SP_NKMd5::end(char *buf)
{
  long i;
  unsigned char digest[16];
  static const char hex[]="0123456789abcdef";
  
  if (!buf)
    return 0;
  final(digest);
  for ( i = 0; i < 16; i++) 
  {
    buf[(i << 1)] = hex[digest[i] >> 4];
    buf[(i << 1) + 1] = hex[digest[i] & 0x0f];
  }

  buf[32] = '\0';

  return buf;
}
#endif

