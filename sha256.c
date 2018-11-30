#include "common.h"

#define ROR(x,n) ((x)>>(n)|(x)<<(32-(n)))

#define HASHTABLE_SIZE 0x40000
#define SEARCHTABLE_SIZE 0x10000

#define BLOCK_SIZE 64

typedef struct
{
    uint32_t hash[8];
    uint32_t id;
} sha_entry_t;

sha_entry_t hashtable[HASHTABLE_SIZE];
sha_entry_t searchtable[SEARCHTABLE_SIZE];
int search_pointer=0;
int current_id=0;

//#pragma SDS data access_pattern(data:SEQUENTIAL)
// #pragma SDS data copy(data[0:length]) //!FLAG maybe a trade off
void sha256_hw(uint8_t data[MAX_LENGTH], uint32_t length, uint32_t output[8])
{
    const uint32_t hashc[8] =
    {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const uint32_t k[64] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t buffer[32];
    uint32_t tmpOutput[8];

    // padding into multiple of 512
    int buffer_number=1;
    int copy_number=(length+9)%BLOCK_SIZE;
    if(copy_number<9)
    {
        copy_number=copy_number+BLOCK_SIZE-9;
        buffer_number=2;
    }
    else
        copy_number-=9;

    for(int a = 0; a < copy_number; a ++)
    {
//non-constant loop bound; cannot unrolled
//variable-indexed range selection may cause suboptimal QoR
//Target II = 3, Final II = 3, Depth = 4. If choose II=2, it exceeds the clock period;
//and there are warnings about critical path
//#pragma HLS PIPELINE II=3
    	((uint8_t *)buffer)[a] = data[length - copy_number + a];
    }


    ((uint8_t *)buffer)[copy_number] = 0x80;
    int zero_number = BLOCK_SIZE - (length + 5) % BLOCK_SIZE;

    for(int b = 0; b  < zero_number; b ++)
    {
//non-constant loop bound; cannot unrolled
//#pragma HLS PIPELINE II=3
    	((uint8_t *)buffer)[copy_number + 1 + b]=0;
    }

    // since length will not go beyond 8K, add uint32_t length instead of standard uint64_t
    buffer[buffer_number==2?31:15]=length*8;

    int chunkn = (length + 9 + zero_number) / 64;

    for(int c = 0; c < 8; c ++)
    {
#pragma HLS unroll
    	tmpOutput[c] = hashc[c];
    }

    for (int d = 0; d < chunkn; d ++)
    {
      //#pragma HLS DATAFLOW
    	//TODO there is data dependence in each chunk processing.
    	// Thus we may fail to pipeline this outer most loop
        uint32_t *chunk;
        if(d >= chunkn - buffer_number)
            chunk = buffer + buffer_number + d - chunkn;
        else
            chunk = (uint32_t *)data + d * 16;
        uint32_t w[64];
        uint32_t temp[8];
#pragma HLS array_partition variable=temp cyclic factor=4
#pragma HLS array_partition variable=w cyclic factor=16
        // copy first 16 words
        for(int e = 0; e < 16; e ++)
        {
#pragma HLS unroll
        	w[e] = chunk[e];
        }

        // calculate extra 48 words
        for(int f = 16; f < 64; f ++)
        {
//TODO:may have data dependency here!! If that is the case, use pipeline with higher II
//#pragma HLS PIPELINE
#pragma HLS unroll
            uint32_t s0 = ROR(w[f-15],7) ^ ROR(w[f-15],18) ^ (w[f-15]>>3);
            uint32_t s1 = ROR(w[f-2],17) ^ ROR(w[f-2],19) ^ (w[f-2]>>10);
            w[f] = w[f-16] + s0 + w[f-7] + s1;
        }

        for(int g = 0; g < 8; g ++)
        {
#pragma HLS unroll
        	temp[g] = tmpOutput[g];
        }


        for(int h = 0; h < 64; h ++)
        {
//TODO may have data dependency here; if fail to get ideal output, can try to use pipeline with higher II, instead of unroll
//#pragma HLS PIPELINE II=4
#pragma HLS unroll
            uint32_t s1 = ROR(temp[4], 6) ^ ROR(temp[4], 11) ^ ROR(temp[4], 25);
            uint32_t ch = (temp[4] & temp[5]) ^ ((~ temp[4]) & temp[6]);
            uint32_t t1 = temp[7] + s1 + ch + k[h] + w[h];
            uint32_t s0 = ROR(temp[0], 2) ^ ROR(temp[0], 13) ^ ROR(temp[0], 22);
            uint32_t mj = (temp[0] & temp[1]) ^ (temp[0] & temp[2]) ^ (temp[1] & temp[2]);
            uint32_t t2 = s0 + mj;

            for(int i = 7; i > 0; i --)
            {
#pragma HLS unroll
                if(i == 4)
                    temp[i] = temp[i-1] + t1;
                else
                    temp[i] = temp[i-1];
            }
            temp[0] = t1 + t2;
        }

        for(int j = 0; j < 8; j ++)
        {
#pragma HLS unroll
        	tmpOutput[j] += temp[j];
        }

    }

    for (int l = 0; l < 8; l ++)
    {
#pragma HLS unroll
    	output[l] = tmpOutput[l];
    }
}


/**
 * returns EXIT_FAILURE(1) for error, EXIT_SUCCESS(0) if the two arrays are the same
 */
int compareSHA(const uint32_t arr1[8], const uint32_t arr2[8])
{
	for (int i = 0; i < 8; i ++)
	{
		if (arr1[i] != arr2[i])
		{
			printf("Compare SHA-256 fails at [index] %d\n", i);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

void sha256_sw(uint8_t *data, uint32_t length, uint32_t *output)
{
    const uint32_t hashc[8] =
    {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const uint32_t k[64] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t buffer[32];
    int i;
    // padding into multiple of 512
    int buffer_number=1;
    int copy_number=(length+9)%BLOCK_SIZE;
    if(copy_number<9)
    {
        copy_number=copy_number+BLOCK_SIZE-9;
        buffer_number=2;
    }
    else
        copy_number-=9;

    for(i=0; i<copy_number; i++)
    {
    	((uint8_t *)buffer)[i]=data[length-copy_number+i];
    }


    ((uint8_t *)buffer)[copy_number]=0x80;
    int zero_number=BLOCK_SIZE-(length+5)%BLOCK_SIZE;
    for(i=0; i<zero_number; i++)
    {
    	((uint8_t *)buffer)[copy_number+1+i]=0;
    }

    // since length will not go beyond 8K, add uint32_t length instead of standard uint64_t
    buffer[buffer_number==2?31:15]=length*8;

    int chunkn=(length+9+zero_number)/64;

    for(i=0; i<8; i++)
    {
    	output[i] = hashc[i];
    }


    for (int c=0; c<chunkn; c++)
    {
        uint32_t *chunk;
        if(c>=chunkn-buffer_number)
            chunk = buffer+buffer_number+c-chunkn;
        else
            chunk = (uint32_t *)data + c * 16;
        uint32_t w[64];
        uint32_t temp[8];
        // copy first 16 words
        for(i=0; i<16; i++)
        {
        	w[i]=chunk[i];
        }

        // calculate extra 48 words
        for(i=16; i<64; i++)
        {
            uint32_t s0 = ROR(w[i-15],7) ^ ROR(w[i-15],18) ^ (w[i-15]>>3);
            uint32_t s1 = ROR(w[i-2],17) ^ ROR(w[i-2],19) ^ (w[i-2]>>10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        for(i=0; i<8; i++)
        {
        	temp[i]=output[i];
        }


        for(i=0; i<64; i++)
        {
            uint32_t s1 = ROR(temp[4], 6) ^ ROR(temp[4], 11) ^ ROR(temp[4], 25);
            uint32_t ch = (temp[4] & temp[5]) ^ ((~ temp[4]) & temp[6]);
            uint32_t t1 = temp[7] + s1 + ch + k[i] + w[i];
            uint32_t s0 = ROR(temp[0], 2) ^ ROR(temp[0], 13) ^ ROR(temp[0], 22);
            uint32_t mj = (temp[0] & temp[1]) ^ (temp[0] & temp[2]) ^ (temp[1] & temp[2]);
            uint32_t t2 = s0 + mj;

            for(int j=7; j>0; j--)
            {
                if(j==4)
                    temp[j]=temp[j-1]+t1;
                else
                    temp[j]=temp[j-1];
            }
            temp[0]=t1+t2;
        }

        for(i=0; i<8; i++)
        {
        	output[i] += temp[i];
        }
    }
}



int32_t lookup(const uint32_t *hash)
{
    unsigned int index = hash[0] % HASHTABLE_SIZE;
    int i = 0;
    if(hashtable[index].hash[0] % HASHTABLE_SIZE == index)
    {
        int found=1;
        for(i=0; i<8; i++)
            if(hashtable[index].hash[i]!=hash[i])
            {
                found=0;
                break;
            }

        if(found) return hashtable[index].id;

        // small hash collision
        for(i=0; i<search_pointer; i++)
        {
            if(searchtable[i].hash[0]==hash[0])
            {
                found=1;
                for(int j=1; j<8; j++)
                    if(searchtable[i].hash[j]!=hash[j])
                    {
                        found=0;
                        break;
                    }

                if(found)
                {
                    found=i;
                    break;
                }
            }
        }

        if(found)
            return searchtable[found].id;
        else
        {
            for(i=0; i<8; i++)
                searchtable[search_pointer].hash[i]=hash[i];
            searchtable[search_pointer].id=current_id;

            search_pointer++;
            current_id++;
            return -1;
        }
    }
    else
    {
        for(i=0; i<8; i++)
            hashtable[index].hash[i]=hash[i];

        hashtable[index].id=current_id;

        current_id++;
        return -1;
    }
}
