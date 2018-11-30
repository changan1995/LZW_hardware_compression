#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif


#define MAX_FILE               0x10000000
#define MAX_LENGTH             0x2000 //8192
#define MIN_LENGTH             0x400 //1024

/**********************************/
/*                                */
/*     CDC[CHUNK VRESION 1]       */
/*                                */
/**********************************/
void Init_CDC_Context(uint64_t *ir, uint64_t *out);

uint32_t Content_Defined_Chunk_sw(const uint8_t *Input,
		                       uint32_t InputLen,
							   uint64_t *ir,
							   uint64_t *out);

uint32_t Content_Defined_Chunk_hw(const uint8_t Input[MAX_FILE],
		                       uint32_t InputLen,
							   uint64_t ir[256],
							   uint64_t out[256]);

/**********************************/
/*                                */
/*              CHUNK             */
/*                                */
/**********************************/
uint32_t rollchunk(uint8_t *input, uint32_t offset, uint32_t length);

/**********************************/
/*                                */
/*      SHA256 CALCULATION        */
/*                                */
/**********************************/
//
// message - the data need to be hashed, the data should be no longer
//           than 8192 bytes, but the array should be at least
//           ((2048+16)*4)=8256 bytes for padding reason
//
// length  - length of data in bytes
//
// output  - sha256 output, should be 32 bytes, i.e. uint32_t[8]
//
void sha256_sw(uint8_t *data, uint32_t length, uint32_t *output);

void sha256_hw(uint8_t data[MAX_LENGTH], uint32_t length, uint32_t output[8]);

/**
 * returns EXIT_FAILURE(1) for error, EXIT_SUCCESS(0) if the two arrays are the same
 */
int compareSHA(const uint32_t arr1[8], const uint32_t arr2[8]);
/**********************************/
/*                                */
/*         SHA256 SEARCH          */
/*                                */
/**********************************/
//
// return id(start from 0) of sha256 if found,
// store the sha256, allocate id and return -1 if not found
//
// hash - hash from sha256
//
int32_t lookup(const uint32_t *hash);

/**********************************/
/*                                */
/*       LZW COMPRESSION          */
/*                                */
/**********************************/
void dict_init(void);
void dict_free(void);

uint32_t lzw_encode(const uint8_t *data, uint32_t length, uint8_t *output);

int compareLZW(const uint8_t * arr1, const uint8_t * arr2, uint32_t length);
/**********************************/
/*                                */
/*         I/O FUNCTIONS          */
/*                                */
/**********************************/
uint32_t load_data(const char *name, unsigned char *data);
void store_data(const char *name, unsigned char *data, unsigned int size);
unsigned char *alloc(int size);
void mfree(void *mem);
void check_error(int error, const char *msg);

#endif
