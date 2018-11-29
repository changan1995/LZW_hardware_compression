#include "common.h"
#include <string.h>

#define ENCODE_FILE "lp.bin"     //"linux.tar" //"lp.bin"//
#define OUTPUT_FILE "output.bin" //"linux.bin" //"output.bin"//

#define CHUNK_SIZE 0x2000
#define F_CPU 667000000

int main()
{
        uint8_t *buf = alloc(MAX_FILE);
        uint8_t *outbuf = alloc(MAX_FILE*0.75);
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

        unsigned long long t[5] = {0};

#ifdef __SDSCC__

        unsigned long long start;
#endif

        // Initializes the Robin Hashing Algorithm
        uint64_t ir[256], out[256];
        uint32_t chunk_sizes[BUCKET_SIZE]; // Parallel group for every chunk
        uint32_t offsets[BUCKET_SIZE + 1]; // offset when processing bucket[i]
        uint8_t* output_buffers[BUCKET_SIZE];
        uint8_t output_sizes[BUCKET_SIZE];
        Init_CDC_Context(ir, out);
        int error = EXIT_SUCCESS;
        while (offset < InputLen - 1)
        {
#ifdef __SDSCC__
                start = sds_clock_counter();
#endif
                //uint32_t chunk_size = rollchunk(buf, offset, InputLen);
                offsets[0] = offset;
                for (int i = 0; i < BUCKET_SIZE; i++)
                {
                        output_buffers[i] = alloc(CHUNK_SIZE); // initialize buffer
                        chunk_sizes[i] = Content_Defined_Chunk(buf, offsets[i], InputLen, ir, out);
                        offsets[i + 1] = offsets[i] + chunk_sizes[i];
                }
// uint32_t chunk_size = Content_Defined_Chunk(buf, offset, InputLen, ir, out);
#ifdef __SDSCC__
                t[0] += sds_clock_counter() - start;
                start = sds_clock_counter();
#endif
// sha256_sw(buf+offset, chunk_size, hash_sw);
#ifdef __SDSCC__
                t[1] += sds_clock_counter() - start;
                start = sds_clock_counter();
#endif
                for (unsigned int i = 0; i < BUCKET_SIZE; i++)
                {
                            sha256_hw(buf+offsets[i], chunk_sizes[i], hash_hw);
// error = compareSHA(hash_sw, hash_hw);
// if (error == EXIT_FAILURE)
// {
// 	printf("Error: SHA-256 HW version produced incorrect value!");
// }
#ifdef __SDSCC__
                        t[2] += sds_clock_counter() - start;
                        start = sds_clock_counter();
#endif
                        int id = lookup(hash_hw);
#ifdef __SDSCC__
                        t[3] += sds_clock_counter() - start;
                        start = sds_clock_counter();
#endif
                        if (id < 0)
                        {
                                // uint32_t lzwInputLen = lzw_encode(buf+offset, chunk_size_bucket[i], outbuf+outoffset+4);
                                uint32_t lzwInputLen = lzw_encode(buf + offsets[i], chunk_sizes[i], output_buffers[i] + 4);
                                uint32_t temp = lzwInputLen << 1;
                                memcpy(output_buffers[i], &temp, sizeof(uint32_t));
                                // memcpy(outbuf + outoffset + 4, output_buffers[i], lzwInputLen);
                                // outoffset += 4 + lzwInputLen;
                                output_sizes[i]  = 4 + lzwInputLen;
                                compressed_size += lzwInputLen;
                        }
                        else
                        {
                                uint32_t temp = id << 1 | 1;
                                memcpy(output_buffers[i], &temp, sizeof(uint32_t));
                                // memcpy(outbuf + outoffset, &temp, sizeof(uint32_t));
                                // outoffset += 4;
                                output_sizes[i] = 4;
                                duplicate_size += chunk_sizes[i];
                        }

                        for(unsigned int i =0; i< BUCKET_SIZE; i++) {
                            memcpy(outbuf + outoffset, output_buffers[i], output_sizes[i]);
                            outoffset += output_sizes[i];
                        }
#ifdef __SDSCC__
                        t[4] += sds_clock_counter() - start;
                        start = sds_clock_counter();
#endif
                        header_size += 4;
                }
                offset = offsets[BUCKET_SIZE];
        }
#ifdef __SDSCC__
        printf("Execution time for CDC: %llu cycles.\n", t[0]);
        printf("Execution time for SHA256_sw: %llu cycles.\n", t[1]);
        printf("Execution time for SHA256_hw: %llu cycles.\n", t[2]);
        printf("Execution time for matching: %llu cycles.\n", t[3]);
        printf("Execution time for LZW: %llu cycles.\n", t[4]);
        printf("Total Execution time: %llu cycles.\n", t[0] + t[2] + t[3] + t[4]);
#endif
        printf("Compressed ratio:%f\n", (double)compressed_size / InputLen);
        printf("De-duplication compress ratio:%f\n", (double)(InputLen - duplicate_size) / InputLen);
        printf("LZW compress ratio:%f\n", (double)compressed_size / (InputLen - duplicate_size));
        printf("Throughput:%f\n", (double)InputLen * F_CPU / (t[0] + t[2] + t[3] + t[4]));

        store_data(OUTPUT_FILE, outbuf, compressed_size + header_size);
        mfree(buf);
        mfree(outbuf);
        // dict_free();
        for (int i = 0; i < BUCKET_SIZE; i++)
        {
                mfree(output_buffers[i]);
        }
        return error;
}
