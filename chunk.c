#include "common.h"

#define PRIME 2377


uint32_t rollchunk(uint8_t *input, uint32_t offset, uint32_t length)
{
    unsigned int chunk_length=0;
    uint64_t queue=0;

    if(length<=8)
        return length;

    // fill the queue
    for(int i=0; i<8-1; i++)
        queue=queue<<8 | input[offset+chunk_length++];

    while(1)
    {
        queue=queue<<8 | input[offset+chunk_length++];
        unsigned int current_hash = queue % PRIME;
        if(((current_hash==1)&& chunk_length>=MIN_LENGTH)||
                (offset+chunk_length)==length || chunk_length==MAX_LENGTH)
            return chunk_length;
    }
}
