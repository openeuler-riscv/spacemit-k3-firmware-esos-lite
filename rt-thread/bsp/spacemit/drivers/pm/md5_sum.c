#include <rtthread.h>
#include <rtservice.h>
#include "md5_sum.h"

static const unsigned int PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void MD5_Transform(unsigned int *state, const unsigned char *block);

// ==========================
// MD5_Init() 实现
// ==========================
void MD5_Init(MD5_CTX *ctx)
{
    ctx->count[0] = 0;
    ctx->count[1] = 0;

    // MD5 初始向量
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

// ==========================
// MD5_Update() 实现
// ==========================
void MD5_Update(MD5_CTX *ctx, const void *input, unsigned long long inputLen)
{
    unsigned long long i, index, partLen;
    const unsigned char *in = (const unsigned char *)input;

    // 已缓冲的字节数
    index = (ctx->count[0] >> 3) & 0x3F;

    // 增加比特长度
    ctx->count[0] += (unsigned int)(inputLen << 3);
    if (ctx->count[0] < (inputLen << 3))
        ctx->count[1]++;
    ctx->count[1] += (unsigned int)(inputLen >> 29);

    partLen = 64 - index;

    if (inputLen >= partLen) {
        rt_memcpy(&ctx->buffer[index], in, partLen);
        MD5_Transform(ctx->state, ctx->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5_Transform(ctx->state, &in[i]);

        index = 0;
    } else {
        i = 0;
    }

    rt_memcpy(&ctx->buffer[index], &in[i], inputLen - i);
}

// ==========================
// MD5_Final() 实现
// ==========================
void MD5_Final(unsigned char *digest, MD5_CTX *ctx)
{
    unsigned char bits[8];
    unsigned long long index, padLen;

    // 保存总比特数
    bits[0] = (unsigned char)((ctx->count[0] >>  0) & 0xFF);
    bits[1] = (unsigned char)((ctx->count[0] >>  8) & 0xFF);
    bits[2] = (unsigned char)((ctx->count[0] >> 16) & 0xFF);
    bits[3] = (unsigned char)((ctx->count[0] >> 24) & 0xFF);
    bits[4] = (unsigned char)((ctx->count[1] >>  0) & 0xFF);
    bits[5] = (unsigned char)((ctx->count[1] >>  8) & 0xFF);
    bits[6] = (unsigned char)((ctx->count[1] >> 16) & 0xFF);
    bits[7] = (unsigned char)((ctx->count[1] >> 24) & 0xFF);

    index = (ctx->count[0] >> 3) & 0x3F;
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5_Update(ctx, PADDING, padLen);
    MD5_Update(ctx, bits, 8);

    // 输出结果
    for (int i = 0; i < 4; i++) {
        digest[i*4+0] = (unsigned char)((ctx->state[i] >>  0) & 0xFF);
        digest[i*4+1] = (unsigned char)((ctx->state[i] >>  8) & 0xFF);
        digest[i*4+2] = (unsigned char)((ctx->state[i] >> 16) & 0xFF);
        digest[i*4+3] = (unsigned char)((ctx->state[i] >> 24) & 0xFF);
    }
}

// ==========================
// MD5 核心变换函数（内部）
// ==========================
static void MD5_Transform(unsigned int *state, const unsigned char *block)
{
    unsigned int a = state[0], b = state[1], c = state[2], d = state[3];
    unsigned int x[16];

    for (int i = 0; i < 16; i++)
        x[i] = (unsigned int)block[i*4+0] | ((unsigned int)block[i*4+1] << 8)
             | ((unsigned int)block[i*4+2] << 16) | ((unsigned int)block[i*4+3] << 24);

    FF(a,b,c,d,x[0],  7,0xD76AA478);
    FF(d,a,b,c,x[1], 12,0xE8C7B756);
    FF(c,d,a,b,x[2], 17,0x242070DB);
    FF(b,c,d,a,x[3], 22,0xC1BDCEEE);
    FF(a,b,c,d,x[4],  7,0xF57C0FAF);
    FF(d,a,b,c,x[5], 12,0x4787C62A);
    FF(c,d,a,b,x[6], 17,0xA8304613);
    FF(b,c,d,a,x[7], 22,0xFD469501);
    FF(a,b,c,d,x[8],  7,0x698098D8);
    FF(d,a,b,c,x[9], 12,0x8B44F7AF);
    FF(c,d,a,b,x[10],17,0xFFFF5BB1);
    FF(b,c,d,a,x[11],22,0x895CD7BE);
    FF(a,b,c,d,x[12], 7,0x6B901122);
    FF(d,a,b,c,x[13],12,0xFD987193);
    FF(c,d,a,b,x[14],17,0xA679438E);
    FF(b,c,d,a,x[15],22,0x49B40821);

    GG(a,b,c,d,x[1],  5,0xF61E2562);
    GG(d,a,b,c,x[6],  9,0xC040B340);
    GG(c,d,a,b,x[11],14,0x265E5A51);
    GG(b,c,d,a,x[0], 20,0xE9B6C7AA);
    GG(a,b,c,d,x[5],  5,0xD62F105D);
    GG(d,a,b,c,x[10], 9,0x02441453);
    GG(c,d,a,b,x[15],14,0xD8A1E681);
    GG(b,c,d,a,x[4], 20,0xE7D3FBC8);
    GG(a,b,c,d,x[9],  5,0x21E1CDE6);
    GG(d,a,b,c,x[14], 9,0xC33707D6);
    GG(c,d,a,b,x[3], 14,0xF4D50D87);
    GG(b,c,d,a,x[8], 20,0x455A14ED);
    GG(a,b,c,d,x[13], 5,0xA9E3E905);
    GG(d,a,b,c,x[2],  9,0xFCEFA3F8);
    GG(c,d,a,b,x[7], 14,0x676F02D9);
    GG(b,c,d,a,x[12],20,0x8D2A4C8A);

    HH(a,b,c,d,x[5],  4,0xFFFA3942);
    HH(d,a,b,c,x[8], 11,0x8771F681);
    HH(c,d,a,b,x[11],16,0x6D9D6122);
    HH(b,c,d,a,x[14],23,0xFDE5380C);
    HH(a,b,c,d,x[1],  4,0xA4BEEA44);
    HH(d,a,b,c,x[4], 11,0x4BDECFA9);
    HH(c,d,a,b,x[7], 16,0xF6BB4B60);
    HH(b,c,d,a,x[10],23,0xBEBFBC70);
    HH(a,b,c,d,x[13], 4,0x289B7EC6);
    HH(d,a,b,c,x[0], 11,0xEAA127FA);
    HH(c,d,a,b,x[3], 16,0xD4EF3085);
    HH(b,c,d,a,x[6], 23,0x04881D05);
    HH(a,b,c,d,x[9],  4,0xD9D4D039);
    HH(d,a,b,c,x[12],11,0xE6DB99E5);
    HH(c,d,a,b,x[15],16,0x1FA27CF8);
    HH(b,c,d,a,x[2], 23,0xC4AC5665);

    II(a,b,c,d,x[0],  6,0xF4292244);
    II(d,a,b,c,x[7], 10,0x432AFF97);
    II(c,d,a,b,x[14],15,0xAB9423A7);
    II(b,c,d,a,x[5], 21,0xFC93A039);
    II(a,b,c,d,x[12], 6,0x655B59C3);
    II(d,a,b,c,x[3], 10,0x8F0CCC92);
    II(c,d,a,b,x[10],15,0xFFEFF47D);
    II(b,c,d,a,x[1], 21,0x85845DD1);
    II(a,b,c,d,x[8],  6,0x6FA87E4F);
    II(d,a,b,c,x[15],10,0xFE2CE6E0);
    II(c,d,a,b,x[6], 15,0xA3014314);
    II(b,c,d,a,x[13],21,0x4E0811A1);
    II(a,b,c,d,x[4],  6,0xF7537E82);
    II(d,a,b,c,x[11],10,0xBD3AF235);
    II(c,d,a,b,x[2], 15,0x2AD7D2BB);
    II(b,c,d,a,x[9], 21,0xEB86D391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}
