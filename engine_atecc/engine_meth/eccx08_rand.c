/**
 *  \file eccx08_rand.c
 * \brief Implementation of OpenSSL ENGINE callback functions
 *        for Random Number Generator
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \atmel_crypto_device_library_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Atmel nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel integrated circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <openssl/engine.h>
#ifdef OPENSSL_DEVEL
    #include <evp_int.h>
#endif // OPENSSL_DEVEL
#include <evp.h>
#include <ossl_typ.h>
#include <openssl/rand.h>
#include "ecc_meth.h"

static int total_num = 0;

/**
 *
 * \brief Generates a random bytes stream. The ATECCX08 TRNG is
 *        used to seed a standard OpenSSL PRNG (RAND_SSLeay()),
 *        which is used to produce the random stream then. The
 *        PRNG is reseeded after MAX_RAND_BYTES are generated
 *
 * \param[out] buf - a pointer to buffer for the random byte
 *       stream. The caller must allocate enough space in the
 *       buffer in order to fit all generated bytes.
 * \param[in] num - number of bytes to generate
 * \return 1 for success
 */
static int RAND_eccx08_rand_bytes(unsigned char *buf, int num)
{
    int rc = 0;
    uint32_t atcab_buf[TLS_RANDOM_SIZE / sizeof(uint32_t)];
    double entropy;
    RAND_METHOD *meth_rand = RAND_SSLeay();
    ATCA_STATUS status = ATCA_GEN_FAIL;

#ifdef USE_ECCX08
    if (total_num > MAX_RAND_BYTES) {
        total_num = 0;
    }
    if (total_num == 0) {
        eccx08_debug("RAND_eccx08_rand_bytes() -  hw\n");
        status = atcatls_init(pCfg);
        if (status != ATCA_SUCCESS) goto done;
        status = atcatls_random((uint8_t *)atcab_buf);
        if (status != ATCA_SUCCESS) goto done;
        status = atcatls_finish();
        if (status != ATCA_SUCCESS) goto done;
        entropy = (double)atcab_buf[0];
        meth_rand->add(buf, num, entropy);
    }
    total_num += num;
#else // USE_ECCX08
    eccx08_debug("RAND_eccx08_rand_bytes() - sw\n");
#endif // USE_ECCX08
    rc = meth_rand->bytes(buf, num);

done:
    return (rc);
}

/**
 *
 * \brief Return success if totally generated number of random
 *        bytes after the last reseed is less than
 *        MAX_RAND_BYTES
 *
 * \return 1 for success
 */
static int RAND_eccx08_rand_status(void)
{
    eccx08_debug("RAND_eccx08_rand_status()\n");
    if (total_num > MAX_RAND_BYTES) {
        return 0;
    }
    return 1;
}

/**
 *  \brief eccx08_rand is an OpenSSL RAND_METHOD structure
 *         specific to the ateccx08 engine.
 *         See the crypto/rand/rand.h file for details on the
 *         struct rand_meth_st
 */
RAND_METHOD eccx08_rand = {  // see crypto/rand/rand.h struct rand_meth_st
    NULL,                   // seed()
    RAND_eccx08_rand_bytes, // bytes()
    NULL,                   // cleanup()
    NULL,                   // add()
    RAND_eccx08_rand_bytes, // pseudorand()
    RAND_eccx08_rand_status // status()
};

/**
 *
 * \brief Initialize the RAND method for ateccx08 engine
 *
 * \return 1 for success
 */
int eccx08_rand_init(void)
{
    const RAND_METHOD *meth_rand = RAND_SSLeay();

    eccx08_debug("eccx08_rand_init()\n");

    /*
     * We use OpenSSL (SSLeay) meth to supply what we don't provide ;-*)
     */
    eccx08_rand.seed = meth_rand->seed;
    eccx08_rand.cleanup = meth_rand->cleanup;

#ifndef USE_ECCX08
    eccx08_rand.bytes = meth_rand->bytes;
    eccx08_rand.add = meth_rand->add;
    eccx08_rand.pseudorand = meth_rand->pseudorand;
    eccx08_rand.status = meth_rand->status;
#endif // USE_ECCX08

    return 1;
}


