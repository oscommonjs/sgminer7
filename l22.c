/*-
 * Copyright 2014 James Lovejoy
 * Copyright 2014 phm
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "miner.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sph/sph_blake.h"
#include "sph/sph_groestl.h"
#include "sph/sph_skein.h"
#include "sph/sph_keccak.h" 
#include "sph/sph_bmw.h"
#include "sph/sph_cubehash.h"
#include "sph/sph_whirlpool.h"
#include "l2.h"

/*
 * Encode a length len/4 vector of (uint32_t) into a length len vector of
 * (unsigned char) in big-endian form.  Assumes len is a multiple of 4.
 */
static inline void
be32enc_vect(uint32_t *dst, const uint32_t *src, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		dst[i] = htobe32(src[i]);
}


inline void dohash(void *state, const void *input)
{
    sph_luffa512_context    luffa1;
    sph_luffa512_init(&luffa1);
    
    uint32_t hashA[16];  
    //luffa-cubehash-shivite-simd-echo
    
    sph_luffa512 (&luffa1, input, 80);
    sph_luffa512_close (&luffa1, hashA);    
        

    memcpy(state, hashA, 32);
}

void whhash(void *state, const void *input)
{
    sph_whirlpool1_context    whirlpool1,whirlpool2,whirlpool3,whirlpool4;
	sph_whirlpool1_init(&whirlpool1);
	sph_whirlpool1_init(&whirlpool2);
	sph_whirlpool1_init(&whirlpool3);
	sph_whirlpool1_init(&whirlpool4);
    
    uint32_t hashA[16], hashB[16];  
    sph_whirlpool1 (&whirlpool1, input, 80);
    sph_whirlpool1_close (&whirlpool1, hashA);      
    
	sph_whirlpool1(&whirlpool2, hashA, 64);
    sph_whirlpool1_close(&whirlpool2, hashB);

    sph_whirlpool1(&whirlpool3, hashB, 64);
    sph_whirlpool1_close(&whirlpool3, hashA);
	
	sph_whirlpool1(&whirlpool4, hashA, 64);
    sph_whirlpool1_close(&whirlpool4, hashB);
	
    memcpy(state, hashB, 32);
}

void whmid(unsigned char *out, const unsigned char *in)
{
	sph_whirlpool1_context    whirlpool1;
	sph_whirlpool1_init(&whirlpool1);
	sph_whirlpool1 (&whirlpool1, in, 80);
	memcpy(out, whirlpool1.state, 64);
}

void wxhash(void *state, const void *input)
{
    unsigned char hash[64];

    memset(hash, 0, sizeof(hash));
    
    sph_whirlpool_context ctx_whirlpool;

    sph_whirlpool_init(&ctx_whirlpool);
    sph_whirlpool(&ctx_whirlpool, input, 80);
    sph_whirlpool_close(&ctx_whirlpool, hash);

    unsigned char hash_xored[sizeof(hash) / 2];

    unsigned int i = 0;
    
    for (; i < (sizeof(hash) / 2); i++)
    {
        hash_xored[i] =
            hash[i] ^ hash[i + ((sizeof(hash) / 2) / 2)]
        ;
    }
    
    memcpy(state, hash_xored, 32);
}

void wxmid(unsigned char *out, const unsigned char *in)
{
	sph_whirlpool_context ctx_whirlpool;
	sph_whirlpool_init(&ctx_whirlpool);
	sph_whirlpool(&ctx_whirlpool, in, 80);
	memcpy(out, ctx_whirlpool.state, 64);
}

inline void l2rehash(void *state, const void *input)
{
    sph_blake256_context     ctx_blake;
    sph_bmw256_context       ctx_bmw;
    sph_keccak256_context    ctx_keccak;
    sph_skein256_context     ctx_skein;
    sph_cubehash256_context  ctx_cube;
    uint32_t hashA[8], hashB[8];

    sph_blake256_init(&ctx_blake);
    sph_blake256 (&ctx_blake, input, 80);
    sph_blake256_close (&ctx_blake, hashA);

    sph_keccak256_init(&ctx_keccak);
    sph_keccak256 (&ctx_keccak,hashA, 32);
    sph_keccak256_close(&ctx_keccak, hashB);

	sph_cubehash256_init(&ctx_cube);
	sph_cubehash256(&ctx_cube, hashB, 32);
	sph_cubehash256_close(&ctx_cube, hashA);

	LYRA2(hashB, 32, hashA, 32, hashA, 32, 1, 4, 4);

	sph_skein256_init(&ctx_skein);
    sph_skein256 (&ctx_skein, hashB, 32);
    sph_skein256_close(&ctx_skein, hashA);

	sph_cubehash256_init(&ctx_cube);
	sph_cubehash256(&ctx_cube, hashA, 32);
	sph_cubehash256_close(&ctx_cube, hashB);

    sph_bmw256_init(&ctx_bmw);
    sph_bmw256 (&ctx_bmw, hashB, 32);
    sph_bmw256_close(&ctx_bmw, hashA);

//printf("cpu hash %08x %08x %08x %08x\n",hashA[0],hashA[1],hashA[2],hashA[3]);

	memcpy(state, hashA, 32);
}

static const uint32_t diff1targ = 0x0000ffff;


void l2_regenhash(struct work *work)
{
        uint32_t data[20];
        uint32_t *nonce = (uint32_t *)(work->data + 76);
        uint32_t *ohash = (uint32_t *)(work->hash);

        be32enc_vect(data, (const uint32_t *)work->data, 19);
        data[19] = htobe32(*nonce);
        l2rehash(ohash, data);
}

void do_regenhash(struct work *work)
{
        uint32_t data[20];
        char *scratchbuf;
        uint32_t *nonce = (uint32_t *)(work->data + 76);
        uint32_t *ohash = (uint32_t *)(work->hash);

        be32enc_vect(data, (const uint32_t *)work->data, 19);
        data[19] = htobe32(*nonce);
        dohash(ohash, data);
}

void wh_regenhash(struct work *work)
{
        uint32_t data[20];
        uint32_t *nonce = (uint32_t *)(work->data + 76);
        uint32_t *ohash = (uint32_t *)(work->hash);

        be32enc_vect(data, (const uint32_t *)work->data, 19);
        data[19] = htobe32(*nonce);
        whhash(ohash, data);
}

void wx_regenhash(struct work *work)
{
        uint32_t data[20];
        uint32_t *nonce = (uint32_t *)(work->data + 76);
        uint32_t *ohash = (uint32_t *)(work->hash);

        be32enc_vect(data, (const uint32_t *)work->data, 19);
        data[19] = htobe32(*nonce);
        wxhash(ohash, data);
}
