#include "common.h"

void check_error(int error, const char *msg)
{
	if(error)
	{
		fputs(msg, stderr);
		exit(EXIT_FAILURE);
	}
}

void exit_with_error(void)
{
	perror(NULL);
	exit(EXIT_FAILURE);
}

unsigned char *alloc(int size)
{
	unsigned char *mem = (unsigned char *)
#ifdef __SDSCC__
    		sds_alloc(size);
#else
	malloc(size);
#endif
	check_error(mem == NULL, "Could not allocate memory.\n");

	return mem;
}

void mfree(void *mem)
{
#ifdef __SDSCC__
	sds_free(mem);
#else
	free(mem);
#endif
}

uint32_t load_data(const char *name, unsigned char *data)
{
	unsigned int size = MAX_FILE;
	unsigned int len;

#ifdef __SDSCC__
	FIL f;

	FRESULT result = f_open(&f, name, FA_READ);
	check_error(result != FR_OK, "Could not open input file.");

	result = f_read(&f, data, size, &len);
	check_error(result != FR_OK, "Could not read input file.");

	check_error(f_close(&f) != FR_OK, "Could not close input file.");

#else
	FILE *fp = fopen(name, "rb");
	if (fp == NULL)
		exit_with_error();

	if (!(len=fread(data, 1, size, fp)))
		exit_with_error();

	if (fclose(fp) != 0)
		exit_with_error();
#endif
	return len;
}

void store_data(const char *name, unsigned char *data, unsigned int size)
{
#ifdef __SDSCC__
	FIL f;
	unsigned int len;

	FRESULT result = f_open(&f, name, FA_WRITE | FA_CREATE_ALWAYS);
	check_error(result != FR_OK, "Could not open output file.");

	result = f_write(&f, data, size, &len);
	check_error(result != FR_OK || len != size, "Could not write output file.");

	check_error(f_close(&f) != FR_OK, "Could not close output file.");
#else
	FILE * fp = fopen(name, "wb");
	if (fp == NULL)
		exit_with_error();

	if (fwrite(data, 1, size, fp) != size)
		exit_with_error();

	if (fclose(fp) != 0)
		exit_with_error();
#endif
}
