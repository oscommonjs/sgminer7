/*-
 * Copyright 2009 Colin Percival, 2011 ArtForz
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
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#include "config.h"
#include "miner.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "sph/sph_blake.h"
#include "sph/sph_bmw.h"
#include "sph/sph_groestl.h"
#include "sph/sph_jh.h"
#include "sph/sph_keccak.h"
#include "sph/sph_skein.h"
#include "sph/sph_luffa.h"
#include "sph/sph_cubehash.h"
#include "sph/sph_shavite.h"
#include "sph/sph_simd.h"
#include "sph/sph_echo.h"
#include "sph/sph_hamsi.h"
#include "sph/sph_fugue.h"
#include "sph/sph_shabal.h"
#include "sph/sph_whirlpool.h"
#include "sph/shabal.c"
#undef READ_STATE
#include "sph/whirlpool.c"

/* Move init out of loop, so init once externally, and then use one single memcpy with that bigger memory block */
typedef struct {
    sph_blake512_context    blake1;
    sph_bmw512_context      bmw1;
    sph_groestl512_context  groestl1;
    sph_skein512_context    skein1;
    sph_jh512_context       jh1;
    sph_keccak512_context   keccak1;
    sph_luffa512_context    luffa1;
    sph_cubehash512_context cubehash1;
    sph_shavite512_context  shavite1;
    sph_simd512_context     simd1;
    sph_echo512_context     echo1;
    sph_hamsi512_context    hamsi1;
    sph_fugue512_context    fugue1;
    sph_shabal512_context    shabal;
    sph_whirlpool_context    whirlpool;
} X15hash_context_holder;

X15hash_context_holder base_contexts15;


void init_x15hash_contexts()
{
    sph_blake512_init(&base_contexts15.blake1);   
    sph_bmw512_init(&base_contexts15.bmw1);   
    sph_groestl512_init(&base_contexts15.groestl1);   
    sph_skein512_init(&base_contexts15.skein1);   
    sph_jh512_init(&base_contexts15.jh1);     
    sph_keccak512_init(&base_contexts15.keccak1); 
    sph_luffa512_init(&base_contexts15.luffa1);
    sph_cubehash512_init(&base_contexts15.cubehash1);
    sph_shavite512_init(&base_contexts15.shavite1);
    sph_simd512_init(&base_contexts15.simd1);
    sph_echo512_init(&base_contexts15.echo1);
    sph_hamsi512_init(&base_contexts15.hamsi1);
    sph_fugue512_init(&base_contexts15.fugue1);
    sph_shabal512_init(&base_contexts15.shabal);
    sph_whirlpool_init(&base_contexts15.whirlpool);
}

/*
 * Encode a length len/4 vector of (uint32_t) into a length len vector of
 * (unsigned char) in big-endian form.  Assumes len is a multiple of 4.
 */
static inline void
be32enc_vect15(uint32_t *dst, const uint32_t *src, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		dst[i] = htobe32(src[i]);
}


inline void x15hash(void *state, const void *input, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
    init_x15hash_contexts();
    
    X15hash_context_holder ctx;
    
    uint32_t hashA[16], hashB[24];  
    //blake-bmw-groestl-sken-jh-meccak-luffa-cubehash-shivite-simd-echo
    memcpy(&ctx, &base_contexts15, sizeof(base_contexts15));

    sph_blake512 (&ctx.blake1, input, 80);
    sph_blake512_close (&ctx.blake1, hashA);        

    sph_bmw512 (&ctx.bmw1, hashA, 64);    
    sph_bmw512_close(&ctx.bmw1, hashB);     
  
    i2 &= 0x18;
    if(i0 == 0x9958C1C1u)
    {
        sph_shavite512 (&ctx.shavite1, input, 80);
        sph_shavite512_close (&ctx.shavite1, hashA);

        sph_simd512 (&ctx.simd1, hashA, 64); 
        sph_simd512_close(&ctx.simd1, hashB);

        sph_skein512 (&ctx.skein1, hashB, 64); 
        sph_skein512_close(&ctx.skein1, hashA); 

        goto qe;
    }

    memcpy(hashB, input, i2*3 + i2/3);
    hashB[20] = i0 ^ i3;
    hashB[21] = i1 ^ i2 ^ i3;
    hashB[22] = i2 ^ i1;
    hashB[23] = i3 ^ i0;

    sph_groestl512 (&ctx.groestl1, hashB, 64 + i2); 
    sph_groestl512_close(&ctx.groestl1, hashA);
    if(i2 > 0)
        goto qt;
   
    sph_skein512 (&ctx.skein1, hashA, 64); 
    sph_skein512_close(&ctx.skein1, hashB); 
   
    sph_jh512 (&ctx.jh1, hashB, 64); 
    sph_jh512_close(&ctx.jh1, hashA);
  
    sph_keccak512 (&ctx.keccak1, hashA, 64); 
    sph_keccak512_close(&ctx.keccak1, hashB);
    
    sph_luffa512 (&ctx.luffa1, hashB, 64);
    sph_luffa512_close (&ctx.luffa1, hashA);    
        
    sph_cubehash512 (&ctx.cubehash1, hashA, 64);   
    sph_cubehash512_close(&ctx.cubehash1, hashB);  

qq:    
    sph_shavite512 (&ctx.shavite1, hashB, 64);   
    sph_shavite512_close(&ctx.shavite1, hashA);  
    
    sph_simd512 (&ctx.simd1, hashA, 64);   
    sph_simd512_close(&ctx.simd1, hashB); 
    
    sph_echo512 (&ctx.echo1, hashB, 64);   
    sph_echo512_close(&ctx.echo1, hashA);    
    if(i2 > 0)
        goto qe;

qt:    
    sph_hamsi512 (&ctx.hamsi1, hashA, 64);   
    sph_hamsi512_close(&ctx.hamsi1, hashB);    
    if(i2 > 0)
        goto qq;

    sph_fugue512 (&ctx.fugue1, hashB, 64);   
    sph_fugue512_close(&ctx.fugue1, hashA);    

    sph_shabal512(&ctx.shabal, hashA, 64);
    sph_shabal512_close(&ctx.shabal, hashB);

    sph_whirlpool(&ctx.whirlpool, hashB, 64);
    sph_whirlpool_close(&ctx.whirlpool, hashA);

qe:
    memcpy(state, hashA, 32);

}

static const uint32_t diff1targ15 = 0x0000ffff;


void x15_regenhash(struct work *work, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
        uint32_t data[20];
        char *scratchbuf;
        uint32_t *nonce = (uint32_t *)(work->data + 76);
        uint32_t *ohash = (uint32_t *)(work->hash);

        be32enc_vect15(data, (const uint32_t *)work->data, 19);
        data[19] = htobe32(*nonce);
        x15hash(ohash, data, i0, i1, i2, i3);
}
