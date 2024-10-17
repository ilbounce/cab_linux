/*
 * sha384.c
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  WjCryptLib_Sha256
//
//  Implementation of SHA256 hash function.
//  Original author: Tom St Denis, tomstdenis@gmail.com, http://libtom.org
//  Modified by WaterJuice retaining Public Domain license.
//
//  This is free and unencumbered software released into the public domain -
//  June 2013 waterjuice.org
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  IMPORTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "sha384.h"
#include <memory.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  MACROS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ADD_UINT128_UINT64(low,high,toadd) do {\
	(low) += (toadd);\
	if((low) < (toadd)){\
		(high)++;\
	}\
} while(0)

#define PUT_UINT128_BE(low,high,b,i) do {\
	PUT_UINT64_BE((high), (b), (i));\
	PUT_UINT64_BE((low), (b), (i)+8);\
} while(0)

#define PUT_MUL8_UINT128_BE(low,high,b,i) do {\
	uint64_t reslow, reshigh;\
	reslow = (low) << 3;\
	reshigh = ((low) >> 61) ^ ((high) << 3);\
	PUT_UINT128_BE(reslow,reshigh,(b),(i));\
} while(0)

#define PUT_UINT64_BE(n,b,i)		\
do {					\
    (b)[(i)    ] = (uint8_t) ( (n) >> 56 );	\
    (b)[(i) + 1] = (uint8_t) ( (n) >> 48 );	\
    (b)[(i) + 2] = (uint8_t) ( (n) >> 40 );	\
    (b)[(i) + 3] = (uint8_t) ( (n) >> 32 );	\
    (b)[(i) + 4] = (uint8_t) ( (n) >> 24 );	\
    (b)[(i) + 5] = (uint8_t) ( (n) >> 16 );	\
    (b)[(i) + 6] = (uint8_t) ( (n) >>  8 );	\
    (b)[(i) + 7] = (uint8_t) ( (n)       );	\
} while( 0 )

#define GET_UINT64_BE(n,b,i)				\
do {							\
    (n) = ( ((uint64_t) (b)[(i)	   ]) << 56 )		\
	| ( ((uint64_t) (b)[(i) + 1]) << 48 )		\
	| ( ((uint64_t) (b)[(i) + 2]) << 40 )		\
	| ( ((uint64_t) (b)[(i) + 3]) << 32 )		\
	| ( ((uint64_t) (b)[(i) + 4]) << 24 )		\
	| ( ((uint64_t) (b)[(i) + 5]) << 16 )		\
	| ( ((uint64_t) (b)[(i) + 6]) <<  8 )		\
	| ( ((uint64_t) (b)[(i) + 7])	    );		\
} while( 0 )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CONSTANTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The K array
static const uint64_t K[80] = {
    (uint64_t)(0x428A2F98D728AE22), (uint64_t)(0x7137449123EF65CD),
    (uint64_t)(0xB5C0FBCFEC4D3B2F), (uint64_t)(0xE9B5DBA58189DBBC),
    (uint64_t)(0x3956C25BF348B538), (uint64_t)(0x59F111F1B605D019),
    (uint64_t)(0x923F82A4AF194F9B), (uint64_t)(0xAB1C5ED5DA6D8118),
    (uint64_t)(0xD807AA98A3030242), (uint64_t)(0x12835B0145706FBE),
    (uint64_t)(0x243185BE4EE4B28C), (uint64_t)(0x550C7DC3D5FFB4E2),
    (uint64_t)(0x72BE5D74F27B896F), (uint64_t)(0x80DEB1FE3B1696B1),
    (uint64_t)(0x9BDC06A725C71235), (uint64_t)(0xC19BF174CF692694),
    (uint64_t)(0xE49B69C19EF14AD2), (uint64_t)(0xEFBE4786384F25E3),
    (uint64_t)(0x0FC19DC68B8CD5B5), (uint64_t)(0x240CA1CC77AC9C65),
    (uint64_t)(0x2DE92C6F592B0275), (uint64_t)(0x4A7484AA6EA6E483),
    (uint64_t)(0x5CB0A9DCBD41FBD4), (uint64_t)(0x76F988DA831153B5),
    (uint64_t)(0x983E5152EE66DFAB), (uint64_t)(0xA831C66D2DB43210),
    (uint64_t)(0xB00327C898FB213F), (uint64_t)(0xBF597FC7BEEF0EE4),
    (uint64_t)(0xC6E00BF33DA88FC2), (uint64_t)(0xD5A79147930AA725),
    (uint64_t)(0x06CA6351E003826F), (uint64_t)(0x142929670A0E6E70),
    (uint64_t)(0x27B70A8546D22FFC), (uint64_t)(0x2E1B21385C26C926),
    (uint64_t)(0x4D2C6DFC5AC42AED), (uint64_t)(0x53380D139D95B3DF),
    (uint64_t)(0x650A73548BAF63DE), (uint64_t)(0x766A0ABB3C77B2A8),
    (uint64_t)(0x81C2C92E47EDAEE6), (uint64_t)(0x92722C851482353B),
    (uint64_t)(0xA2BFE8A14CF10364), (uint64_t)(0xA81A664BBC423001),
    (uint64_t)(0xC24B8B70D0F89791), (uint64_t)(0xC76C51A30654BE30),
    (uint64_t)(0xD192E819D6EF5218), (uint64_t)(0xD69906245565A910),
    (uint64_t)(0xF40E35855771202A), (uint64_t)(0x106AA07032BBD1B8),
    (uint64_t)(0x19A4C116B8D2D0C8), (uint64_t)(0x1E376C085141AB53),
    (uint64_t)(0x2748774CDF8EEB99), (uint64_t)(0x34B0BCB5E19B48A8),
    (uint64_t)(0x391C0CB3C5C95A63), (uint64_t)(0x4ED8AA4AE3418ACB),
    (uint64_t)(0x5B9CCA4F7763E373), (uint64_t)(0x682E6FF3D6B2B8A3),
    (uint64_t)(0x748F82EE5DEFB2FC), (uint64_t)(0x78A5636F43172F60),
    (uint64_t)(0x84C87814A1F0AB72), (uint64_t)(0x8CC702081A6439EC),
    (uint64_t)(0x90BEFFFA23631E28), (uint64_t)(0xA4506CEBDE82BDE9),
    (uint64_t)(0xBEF9A3F7B2C67915), (uint64_t)(0xC67178F2E372532B),
    (uint64_t)(0xCA273ECEEA26619C), (uint64_t)(0xD186B8C721C0C207),
    (uint64_t)(0xEADA7DD6CDE0EB1E), (uint64_t)(0xF57D4F7FEE6ED178),
    (uint64_t)(0x06F067AA72176FBA), (uint64_t)(0x0A637DC5A2C898A6),
    (uint64_t)(0x113F9804BEF90DAE), (uint64_t)(0x1B710B35131C471B),
    (uint64_t)(0x28DB77F523047D84), (uint64_t)(0x32CAAB7B40C72493),
    (uint64_t)(0x3C9EBE0A15C9BEBC), (uint64_t)(0x431D67C49C100D4C),
    (uint64_t)(0x4CC5D4BECB3E42B6), (uint64_t)(0x597F299CFC657E2A),
    (uint64_t)(0x5FCB6FAB3AD6FAEC), (uint64_t)(0x6C44198C4A475817) };

#define BLOCK_SIZE 128

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  INTERNAL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Various logical functions

#define CH(x, y, z)	(((x) & (y)) ^ ((~(x)) & (z)))
#define MAJ(x, y, z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SHR_SHA512(x, n)       (((uint64_t)(x)) >> (n))
#define ROTR_SHA512(x, n)      ((((uint64_t)(x)) >> (n)) | (((uint64_t)(x)) << (64-(n))))
#define SIGMA_MAJ0_SHA512(x)   (ROTR_SHA512(x, 28) ^ ROTR_SHA512(x, 34) ^ ROTR_SHA512(x, 39))
#define SIGMA_MAJ1_SHA512(x)   (ROTR_SHA512(x, 14) ^ ROTR_SHA512(x, 18) ^ ROTR_SHA512(x, 41))
#define SIGMA_MIN0_SHA512(x)   (ROTR_SHA512(x, 1)  ^ ROTR_SHA512(x, 8)	^ SHR_SHA512(x, 7))
#define SIGMA_MIN1_SHA512(x)   (ROTR_SHA512(x, 19) ^ ROTR_SHA512(x, 61) ^ SHR_SHA512(x, 6))


#define SHA2CORE_SHA512(a, b, c, d, e, f, g, h, w, k) do {\
	uint64_t t1, t2;\
	t1 = (h) + SIGMA_MAJ1_SHA512((e)) + CH((e), (f), (g)) + (k) + (w);\
	t2 = SIGMA_MAJ0_SHA512((a)) + MAJ((a), (b), (c));\
	(h) = (g);\
	(g) = (f);\
	(f) = (e);\
	(e) = (d) + t1;\
	(d) = (c);\
	(c) = (b);\
	(b) = (a);\
	(a) = t1 + t2;\
} while(0)
#define UPDATEW_SHA512(w, i) ((w)[(i)] = SIGMA_MIN1_SHA512((w)[(i)-2]) + (w)[(i)-7] + SIGMA_MIN0_SHA512((w)[(i)-15]) + (w)[(i)-16])

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TransformFunction
//
//  Compress 512-bits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void TransformFunction(Sha384Context* Context, uint8_t const* Buffer) {
    uint64_t S[8];
    uint64_t W[80];
    int i;

    // Copy state into S
    for (i = 0; i < 8; i++) {
        S[i] = Context->state[i];
    }

    // Copy the state into 512-bits into W[0..15]
    for (i = 0; i < 16; i++) {
        GET_UINT64_BE(W[i], Buffer, 8 * i);
        SHA2CORE_SHA512(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], W[i], K[i]);
    }

    // Fill W[16..79]
    for (i = 16; i < 80; i++) {
        SHA2CORE_SHA512(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], UPDATEW_SHA512(W, i),
            K[i]);
    }

    // Feedback
    for (i = 0; i < 8; i++) {
        Context->state[i] += S[i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha384Initialise
//
//  Initialises a SHA384 Context. Use this to initialise/reset a context.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Sha384Initialise(Sha384Context* Context  // [out]
) {
    Context->total[0] = Context->total[1] = 0;
    Context->state[0] = (uint64_t)(0xCBBB9D5DC1059ED8);
    Context->state[1] = (uint64_t)(0x629A292A367CD507);
    Context->state[2] = (uint64_t)(0x9159015A3070DD17);
    Context->state[3] = (uint64_t)(0x152FECD8F70E5939);
    Context->state[4] = (uint64_t)(0x67332667FFC00B31);
    Context->state[5] = (uint64_t)(0x8EB44A8768581511);
    Context->state[6] = (uint64_t)(0xDB0C2E0D64F98FA7);
    Context->state[7] = (uint64_t)(0x47B5481DBEFA4FA4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha384Update
//
//  Adds data to the SHA384 context. This will process the data and update the
//  internal state of the context. Keep on calling this function until all the
//  data has been added. Then call Sha384Finalise to calculate the hash.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Sha384Update(Sha384Context* Context,  // [in out]
    void const* Buffer,      // [in]
    uint32_t BufferSize      // [in]
) {
    uint32_t left;
    uint32_t fill;
    uint32_t remain = BufferSize;

    const uint8_t* data_ptr = (uint8_t*)Buffer;

    if (Context->total[0] > sizeof(Context->buf)) {
        return;
    }

    left = (Context->total[0] & 0x7F);
    fill = (BLOCK_SIZE - left);

    ADD_UINT128_UINT64(Context->total[0], Context->total[1], BufferSize);

    if ((left > 0) && (remain >= fill)) {
        /* Copy data at the end of the buffer */
        memcpy(Context->buf + left, data_ptr, fill);
        TransformFunction(Context, (uint8_t*)Context->buf);
        data_ptr += fill;
        remain -= fill;
        left = 0;
    }

    while (remain >= BLOCK_SIZE) {
        TransformFunction(Context, data_ptr);
        data_ptr += BLOCK_SIZE;
        remain -= BLOCK_SIZE;
    }

    if (remain > 0) {
        memcpy(Context->buf + left, data_ptr, remain);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha384Finalise
//
//  Performs the final calculation of the hash and returns the digest (32 byte
//  buffer containing 384bit hash). After calling this, Sha384Initialised must
//  be used to reuse the context.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Sha384Finalise(Sha384Context* Context,  // [in out]
    SHA384_HASH* Digest      // [out]
) {

    unsigned int block_present = 0;
    uint8_t last_padded_block[2 * BLOCK_SIZE];

    memset(last_padded_block, 0, sizeof(last_padded_block));

    block_present = (Context->total[0] % BLOCK_SIZE);
    if (block_present != 0) {
        /* Copy what's left in our temporary context buffer */
        memcpy(last_padded_block, (uint8_t*)Context->buf,
            block_present);
    }

    last_padded_block[block_present] = 0x80;


    if (block_present > (BLOCK_SIZE - 1 - (2 * sizeof(uint64_t)))) {
        /* We need an additional block */
        PUT_MUL8_UINT128_BE(Context->total[0], Context->total[1],
            last_padded_block,
            2 * (BLOCK_SIZE - sizeof(uint64_t)));
        TransformFunction(Context, last_padded_block);
        TransformFunction(Context, last_padded_block + BLOCK_SIZE);
    }
    else {
        /* We do not need an additional block */
        PUT_MUL8_UINT128_BE(Context->total[0], Context->total[1],
            last_padded_block,
            BLOCK_SIZE - (2 * sizeof(uint64_t)));
        TransformFunction(Context, last_padded_block);
    }

    /* Output the hash result */
    PUT_UINT64_BE(Context->state[0], Digest->bytes, 0);
    PUT_UINT64_BE(Context->state[1], Digest->bytes, 8);
    PUT_UINT64_BE(Context->state[2], Digest->bytes, 16);
    PUT_UINT64_BE(Context->state[3], Digest->bytes, 24);
    PUT_UINT64_BE(Context->state[4], Digest->bytes, 32);
    PUT_UINT64_BE(Context->state[5], Digest->bytes, 40);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha384Calculate
//
//  Combines Sha384Initialise, Sha384Update, and Sha384Finalise into one
//  function. Calculates the SHA256 hash of the buffer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Sha384Calculate(void const* Buffer,   // [in]
    uint32_t BufferSize,  // [in]
    SHA384_HASH* Digest   // [in]
) {
    Sha384Context context;

    Sha384Initialise(&context);
    Sha384Update(&context, Buffer, BufferSize);
    Sha384Finalise(&context, Digest);
}
