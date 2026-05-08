#ifndef __MD5_FUNC_H__
#define __MD5_FUNC_H__

#include <rtthread.h>

typedef struct {
    unsigned int state[4];         // A B C D
    unsigned int count[2];         // 总比特数
    unsigned char buffer[64];       // 输入缓冲区
} MD5_CTX;

#define MD5_DIGEST_LENGTH 16

#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x ^ y ^ z)
#define I(x,y,z) (y ^ (x | ~z))

#define ROTATE_LEFT(x, n) ((x << n) | (x >> (32 - n)))

#define FF(a,b,c,d,x,s,ac) { a += F(b,c,d) + x + ac; a = ROTATE_LEFT(a,s); a += b; }
#define GG(a,b,c,d,x,s,ac) { a += G(b,c,d) + x + ac; a = ROTATE_LEFT(a,s); a += b; }
#define HH(a,b,c,d,x,s,ac) { a += H(b,c,d) + x + ac; a = ROTATE_LEFT(a,s); a += b; }
#define II(a,b,c,d,x,s,ac) { a += I(b,c,d) + x + ac; a = ROTATE_LEFT(a,s); a += b; }

void MD5_Init(MD5_CTX *ctx);
void MD5_Update(MD5_CTX *ctx, const void *input, unsigned long long inputLen);
void MD5_Final(unsigned char *digest, MD5_CTX *ctx);

#endif
