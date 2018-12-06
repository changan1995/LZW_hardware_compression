#include "common.h"
#include <string.h>
#include <assert.h>

#define ENCODE_FILE "lp.bin"//"linux.tar" //"lp.bin"//
#define OUTPUT_FILE "output.bin"//"linux.bin" //"output.bin"//

#define CHUNK_SIZE 0x2000
#define F_CPU 667000000

/**
 * helper function to get throughput
 */
double getTp(uint32_t InputLen, unsigned long long cycles)
{
	double time = (double)(667 * 1000000) / (double)(cycles);
	uint32_t fileSizeInBit = InputLen * 8;
	double result = (double)fileSizeInBit * time;
	return result;
}
int main()
{
	uint8_t *buf = alloc(MAX_FILE);
	uint8_t *outbuf = alloc(MAX_FILE/2);
	uint32_t offset = 0;
	uint32_t outoffset = 0;

#ifdef __SDSCC__
	FATFS FS;

	check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
#endif

	uint32_t InputLen = load_data(ENCODE_FILE, buf);

	uint32_t hash_sw[8];
	uint32_t hash_hw[8];
	uint32_t duplicate_size = 0;
	uint32_t compressed_size = 0;
	uint32_t header_size = 0;

	dict_init();
	unsigned long long t[10]={0};

#ifdef __SDSCC__

	unsigned long long start;
#endif

	// Initializes the Robin Hashing Algorithm
	uint64_t ir[256], out[256];
	Init_CDC_Context(ir, out);
	int error = EXIT_SUCCESS;
	while(offset<InputLen-1)
	{
#ifdef __SDSCC__
		start = sds_clock_counter();
#endif
		//uint32_t chunk_size = rollchunk(buf, offset, InputLen);
		uint32_t chunk_size_sw = Content_Defined_Chunk_sw(buf+offset, InputLen-offset, ir, out);
#ifdef __SDSCC__
		t[0] += sds_clock_counter() - start;
		start = sds_clock_counter();
#endif
		//sha256_sw(buf+offset, chunk_size_sw, hash_sw);
		SHA_256(buf+offset, chunk_size_sw, hash_sw);
#ifdef __SDSCC__
		t[8] += sds_clock_counter() - start;
		start = sds_clock_counter();
#endif
		SHA_256_hw(buf+offset, chunk_size_sw, hash_hw);
		if (compareSHA(hash_sw, hash_hw))
		{
			error = 1;
			printf("Error: SHA-256 HW version produced incorrect value; offset: %lu!\n", offset);
		}
#ifdef __SDSCC__
		t[1] += sds_clock_counter() - start;
		start = sds_clock_counter();
#endif
		int id = lookup(hash_sw);
#ifdef __SDSCC__
		t[2] += sds_clock_counter() - start;
		start = sds_clock_counter();
#endif
		if(id < 0)
		{
			uint32_t lzwInputLen = lzw_encode(buf+offset, chunk_size_sw, outbuf+outoffset+4);
			uint32_t temp = lzwInputLen << 1;
			memcpy(outbuf + outoffset, &temp, sizeof(uint32_t));
			outoffset += 4 + lzwInputLen;
			compressed_size += lzwInputLen;
		}
		else
		{
			uint32_t temp = id << 1|1;
			memcpy(outbuf + outoffset, &temp, sizeof(uint32_t));
			outoffset += 4;
			duplicate_size += chunk_size_sw;
		}
#ifdef __SDSCC__
		t[3] += sds_clock_counter() - start;
		start = sds_clock_counter();
#endif
		header_size += 4;
		offset += chunk_size_sw;
	}
#ifdef __SDSCC__
	printf("Execution time for CDC_sw: %llu cycles; throughput: %f\n", t[0], getTp(InputLen, t[0]));
	printf("Execution time for SHA256_sw: %llu cycles; throughput: %f\n", t[8], getTp(InputLen, t[8]));
	printf("Execution time for SHA256_hw: %llu cycles; throughput: %f\n", t[1], getTp(InputLen, t[1]));
	printf("Execution time for matching: %llu cycles; throughput: %f\n", t[2], getTp(InputLen, t[2]));
	printf("Execution time for LZW: %llu cycles; throughput: %f\n", t[3], getTp(InputLen, t[3]));
	printf("Total Execution time: %llu cycles.\n", t[0]+t[1]+t[2]+t[3]);
#endif
	printf("Input file size in Byte: %lu\n", InputLen);
	printf("Compressed ratio:%f\n", (double)outoffset/InputLen);
	printf("De-duplication compress ratio:%f\n", (double)(InputLen-duplicate_size)/InputLen);
	printf("LZW compress ratio:%f\n", (double)compressed_size/(InputLen-duplicate_size));
	printf("Throughput:%f\n", (double)InputLen*F_CPU/(t[0]+t[1]+t[2]+t[3]));

	store_data(OUTPUT_FILE, outbuf, compressed_size + header_size);
	mfree(buf);
	mfree(outbuf);
	dict_free();

	return error;
}
