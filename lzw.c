#include <string.h>
#include "common.h"

#define LZW_DICT_SIZE 0x2100
#define CODE_LENGTH 13

typedef struct
{
    uint16_t table[256];
} lzw_dict_t;


// void dict_init(void)
// {
// }

// void dict_free(void)
// {
// 	mfree(lzwdict);
// }

static void lzw_save(uint16_t val, unsigned int *remain, unsigned int *offset, uint8_t output[MAX_LENGTH])
{   

    
    unsigned int bits = CODE_LENGTH;
    while(bits>0)
        if(bits>=*remain)
        {
            output[*offset]&=0xFF<<*remain;
            output[(*offset)++]|=(uint8_t)(val>>(bits-*remain));
            val&=0xFFFF>>(16-bits);
            bits-=*remain;
            *remain=8;
        }
        else
        {
            output[*offset]&=0xFF>>bits;
            output[*offset]|=(uint8_t)(val<<(8-bits));
            *remain=8-bits;
            bits=0;
        }
}

#pragma SDS data zero_copy(data)
#pragma SDS data access_pattern(data:SEQUENTIAL)
uint32_t lzw_encode(const uint8_t data[MAX_LENGTH], uint32_t length, uint8_t output[MAX_LENGTH])
{
    unsigned int remain_bits=8;
    unsigned int data_pointer=1;
    unsigned int output_pointer=0;
    unsigned int current_dict=data[0];
    unsigned int dict_pointer=256;

    
    lzw_dict_t lzwdict[LZW_DICT_SIZE];
    // lzwdict=(lzw_dict_t *)alloc(sizeof(lzw_dict_t)*LZW_DICT_SIZE);
    // memset(lzwdict, 0, sizeof(lzw_dict_t)*LZW_DICT_SIZE);

    while(data_pointer<length)
    {
        if(lzwdict[current_dict].table[data[data_pointer]])
            current_dict=lzwdict[current_dict].table[data[data_pointer]];
        else
        {
            lzwdict[current_dict].table[data[data_pointer]] = dict_pointer++;
            lzw_save(current_dict, &remain_bits, &output_pointer, output);
            current_dict=data[data_pointer];
        }
        data_pointer++;
    }

    lzw_save(current_dict, &remain_bits, &output_pointer, output);
    // memset(lzwdict, 0, sizeof(lzw_dict_t) * dict_pointer);
    // mfree(lzwdict);
    return output_pointer+(remain_bits==8?0:1);
}
