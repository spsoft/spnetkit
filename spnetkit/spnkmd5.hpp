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
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#ifndef __spnkmd5_hpp__
#define __spnkmd5_hpp__

class SP_NKMd5
{
public:
  static int md5sum( unsigned char const * buf, int buflen, char * hex, int len );

public:
  SP_NKMd5();
  ~SP_NKMd5();
  void init();
  void update(unsigned char const *buf, unsigned long len);
  void final(unsigned char digest[16]);
  char* end(char *buf);
private:
  unsigned int buf[4];
  unsigned int bytes[2];
  unsigned int in[16];
  void transform(unsigned int buf[4], unsigned int const in[16]);
};

#endif

