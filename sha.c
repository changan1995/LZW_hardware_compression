/*
 * sha.c
 *  Source Code Reference: https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c
 *  Created on: Nov 7, 2018
 *      Author: xinyu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/********************** MACROS AND STRUCTURES ***********************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	uint64_t bitlen;
	uint32_t state[8];
} SHA256_CTX;

/**************************** VARIABLES *****************************/
static const uint32_t k[64] = {
		0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
		0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
		0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
		0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
		0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
		0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
		0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
		0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*********************** FUNCTION DEFINITIONS ***********************/

/**
 * Compression function main part
 */
static void sha256_transform(SHA256_CTX *ctx, const uint8_t *data)
{
	uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

/**
 * Initializes the state array of the context structure
 */
static void sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

/**
 * Reads in the chunk serially
 * Breaks input chunk into 512-bit (32-bit * 16) blocks
 */
static void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len)
{
	uint32_t i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

/**
 * Processes the last part
 */
static void sha256_final(SHA256_CTX *ctx, uint32_t *hash)
{
	uint32_t i;

	i = ctx->datalen;
	ctx->data[i++] = 0x80;
	while (i < 64)
	{
		ctx->data[i++] = 0x00;
	}

	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 8; i ++)
	{
		hash[i] = ctx->state[i];
	}
}

/**
 * Implements the SHA-256 algorithm.
 * This implementation uses little endian byte order.
 */
void SHA_256(const uint8_t *InputChunk, uint32_t ChunkLength, uint32_t *currSHA)
{
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, InputChunk, ChunkLength);
	sha256_final(&ctx, currSHA);
}

/******************** HARDWARE VERSION *************************/

/**
 * Compression function main part
 */
//#pragma SDS data zero_copy(data[0:64], state[0:8])
//#pragma SDS data access_pattern(data:SEQUENTIAL, state:SEQUENTIAL)
void sha256_transform_hw(uint8_t data[64], uint32_t state[8])
{
	uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
#pragma HLS array_partition variable=m cyclic factor=4

	for (i = 0, j = 0; i < 16; ++i, j += 4)
	{
#pragma HLS unroll
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	}
	for ( ; i < 64; ++i)
	{
#pragma HLS unroll
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	for (i = 0; i < 64; ++i) {
#pragma HLS unroll
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}


/**
 * Implements the SHA-256 algorithm.
 * This implementation uses little endian byte order.
 */
//#pragma SDS data access_pattern(InputChunk:SEQUENTIAL)
//#pragma SDS data copy(InputChunk[0:ChunkLength])
void SHA_256_hw(const uint8_t InputChunk[MAX_LENGTH],
		uint32_t ChunkLength,
		uint32_t currSHA[8])
{
	uint32_t state[8];

	/* Initializes the state array. */
	state[0] = 0x6a09e667;
	state[1] = 0xbb67ae85;
	state[2] = 0x3c6ef372;
	state[3] = 0xa54ff53a;
	state[4] = 0x510e527f;
	state[5] = 0x9b05688c;
	state[6] = 0x1f83d9ab;
	state[7] = 0x5be0cd19;

	uint32_t i;
	uint32_t dataLength = 0;
	uint64_t bitLength = 0;
	uint8_t data[64];
//#pragma HLS array_partition variable=data cyclic factor=32

	/* Reads in the chunk serially */
	/* Breaks input chunk into 512-bit (32-bit * 16) blocks */
	for (i = 0; i < ChunkLength; ++i) {
		data[dataLength] = InputChunk[i];
		dataLength ++;
		if (dataLength == 64) {
			sha256_transform_hw(data, state);
			bitLength += 512;
			dataLength = 0;
		}
	}

	i = dataLength;
	data[i++] = 0x80;
	while (i < 64)
	{
		data[i++] = 0x00;
	}

	sha256_transform_hw(data, state);

	for (i = 0; i < 8; i ++)
	{
#pragma HLS unroll
		currSHA[i] = state[i];
	}
}
