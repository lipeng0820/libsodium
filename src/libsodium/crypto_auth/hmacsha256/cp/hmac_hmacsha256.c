
/*-
 * Copyright 2005,2007,2009 Colin Percival
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
 */

#include "api.h"
#include "crypto_auth_hmacsha256.h"
#include "crypto_hash_sha256.h"
#include "utils.h"

#include <sys/types.h>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void
HMAC__SHA256_Init(crypto_auth_hmacsha256_state * ctx, const void * _K, size_t Klen)
{
    unsigned char pad[64];
    unsigned char khash[32];
    const unsigned char * K = (const unsigned char *) _K;
    size_t i;

    if (Klen > 64) {
        crypto_hash_sha256_init(&ctx->ictx);
        crypto_hash_sha256_update(&ctx->ictx, K, Klen);
        crypto_hash_sha256_final(&ctx->ictx, khash);
        K = khash;
        Klen = 32;
    }
    crypto_hash_sha256_init(&ctx->ictx);
    memset(pad, 0x36, 64);
    for (i = 0; i < Klen; i++) {
        pad[i] ^= K[i];
    }
    crypto_hash_sha256_update(&ctx->ictx, pad, 64);

    crypto_hash_sha256_init(&ctx->octx);
    memset(pad, 0x5c, 64);
    for (i = 0; i < Klen; i++) {
        pad[i] ^= K[i];
    }
    crypto_hash_sha256_update(&ctx->octx, pad, 64);

    sodium_memzero((void *) khash, 32);
}

static void
HMAC__SHA256_Update(crypto_auth_hmacsha256_state * ctx, const void *in, size_t len)
{
    crypto_hash_sha256_update(&ctx->ictx, in, len);
}

static void
HMAC__SHA256_Final(unsigned char digest[32], crypto_auth_hmacsha256_state * ctx)
{
    unsigned char ihash[32];

    crypto_hash_sha256_final(&ctx->ictx, ihash);
    crypto_hash_sha256_update(&ctx->octx, ihash, 32);
    crypto_hash_sha256_final(&ctx->octx, digest);

    sodium_memzero((void *) ihash, 32);
}

int
crypto_auth_hmacsha256_init(crypto_auth_hmacsha256_state *state,
                            const unsigned char *key,
                            const size_t keylen)
{
    HMAC__SHA256_Init(state, key, keylen);

    return 0;
}

int
crypto_auth_hmacsha256_update(crypto_auth_hmacsha256_state *state,
                              const unsigned char *in,
                              unsigned long long inlen)
{
    if (inlen > SIZE_MAX) {
        sodium_memzero((void *) state, sizeof *state);
        return -1;
    }
    HMAC__SHA256_Update(state, (const void *) in, (size_t) inlen);

    return 0;
}

int
crypto_auth_hmacsha256_final(crypto_auth_hmacsha256_state *state,
                             unsigned char *out)
{
    HMAC__SHA256_Final(out, state);

    return 0;
}

int
crypto_auth(unsigned char *out, const unsigned char *in,
            unsigned long long inlen, const unsigned char *k)
{
    crypto_auth_hmacsha256_state ctx;

    if (inlen > SIZE_MAX) {
        sodium_memzero((void *) out, crypto_auth_BYTES);
        return -1;
    }
    HMAC__SHA256_Init(&ctx, k, crypto_auth_KEYBYTES);
    HMAC__SHA256_Update(&ctx, (const void *) in, (size_t) inlen);
    HMAC__SHA256_Final(out, &ctx);

    return 0;
}
