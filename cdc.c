/*
 * cdc.c
 *
 *  Created on: Nov 6, 2018
 *      Author: xinyu
 *
 *  Notes: (https://moinakg.wordpress.com/2013/06/22/high-performance-content-defined-chunking/)
 *  Source Code Reference: https://github.com/moinakg/pcompress/blob/master/rabin/rabin_dedup.c
 *
 *  1. The MODULUS operation can be replaced with masking if it is a power of 2.
 *  2. Since the sliding window is just 16 bytes it is possible to keep it entirely in a 128-bit SSE register.
 *  3. Since we have minimum and maximum limits for chunk sizes, it is possible to skip minlength –
 *  4. small constant bytes after a breakpoint is found and then start scanning.
 *  5. This provides for a significant improvement in performance by avoiding scanning majority of the data stream.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "common.h"

// Macros used in CDC
// Reference: https://github.com/moinakg/pcompress/blob/master/rabin/rabin_dedup.h
#define POLYNOMIAL_WIN_SIZE     16 // 8 ~ 64
#define WINDOW_SLIDE_OFFSET     64
#define POLY_MASK              (0xffffffffffULL) // not care about the bottom 40 bits
#define POLYNOMIAL_CONST        153191 // Use prime constant from Bulat Ziganshin's REP.
                                       // Seems to work best across wide range of data.
#define	RAB_BLK_MIN_BITS        10    // minimum chunk size 1K
#define	RAB_BLK_MASK           (((1 << RAB_BLK_MIN_BITS) - 1) >> 1)
#define	FP_POLY                0xbfe6b8a5bf378d83ULL


/**
 * Pre-compute a table of irreducible polynomial evaluations for each
 * possible byte value.
 *
 */
void Init_CDC_Context(uint64_t *ir, uint64_t *out)
{
	unsigned int term, pow, i, j;
	uint64_t val, poly_pow;
	poly_pow = 1;
	for (j = 0; j < POLYNOMIAL_WIN_SIZE; j++)
	{
		poly_pow = (poly_pow * POLYNOMIAL_CONST) & POLY_MASK;
	}

	for (j = 0; j < 256; j++)
	{
		term = 1;
		pow = 1;
		val = 1;
		out[j] = (j * poly_pow) & POLY_MASK;
		for (i = 0; i < POLYNOMIAL_WIN_SIZE; i++)
		{
			if (term & FP_POLY)
			{
				val += ((pow * j) & POLY_MASK);
			}
			pow = (pow * POLYNOMIAL_CONST) & POLY_MASK;
			term <<= 1;
		}
		ir[j] = val;
	}
}
/**
 * Perform Content defined chunk.
 *
 * Semi-Rabin fingerprinting based Deduplication are supported.
 * A 16-byte window is used for the rolling checksum and dedup blocks can vary in size
 * from 2K-8K.
 *
 * @Return: length of the chunk
 */
uint32_t Content_Defined_Chunk(const uint8_t *Input,
		                       uint32_t InputOffset,
		                       uint32_t InputLen,
							   uint64_t *ir,
							   uint64_t *out)
{
	// check if the remaining part is smaller than the minimum chunk size
	uint32_t remainingLen = InputLen - InputOffset;
	if (remainingLen <= MIN_LENGTH)
	{
		return remainingLen;
	}

	// Sliding window for finding the chunk boundary
	uint8_t currWinData[POLYNOMIAL_WIN_SIZE];
	uint8_t windowPos = 0;
	memset(currWinData, 0, POLYNOMIAL_WIN_SIZE);

	/*------- Main part of the algorithm ---------*/

	// Start our sliding window at a fixed number of bytes before the MIN_LENGTH.
	uint32_t endIndex = InputLen - POLYNOMIAL_WIN_SIZE;
	uint32_t length = MIN_LENGTH - WINDOW_SLIDE_OFFSET;
	uint32_t offset = InputOffset + length;

	uint64_t rollChecksum, posChecksum; //TODO: Do I need such a big value?
	for (uint32_t i = offset; i < endIndex; i ++)
	{
		uint8_t currByte = Input[i];
		uint32_t pushedOut = currWinData[windowPos];
		currWinData[windowPos] = currByte;
		rollChecksum = (rollChecksum * POLYNOMIAL_CONST) & POLY_MASK;
		rollChecksum += currByte;
		rollChecksum -= out[pushedOut];
		windowPos = (windowPos + 1) & (POLYNOMIAL_WIN_SIZE - 1); // windowPos has to rotate from 0 .. POLYNOMIAL_WIN_SIZE-1
		                                                         // We avoid a branch here by masking.

		++ length;
		if (length < MIN_LENGTH)
		{
			continue;
		}

		// If we hit our special value or reached the max block size update block offset
		posChecksum = rollChecksum ^ ir[pushedOut];
		if ((posChecksum & RAB_BLK_MASK) == 0 || length >= MAX_LENGTH)
		{
			return length;
		}
	}

	// Insert the last left-over trailing bytes, if any, into a block.
	length = InputLen - InputOffset;
	return length;
}
