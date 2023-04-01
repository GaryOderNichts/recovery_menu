/*
 *   Copyright (C) 2022-2023 GaryOderNichts, GerbilSoft
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "rsa.h"
#include "mini-gmp.h"
#include "imports.h"

#include <stdint.h>
#include <string.h>

static const uint8_t rsa_pubkey[] = {
    0xc4, 0x6d, 0xf0, 0x29, 0x58, 0x1c, 0x57, 0x9c, 0x9d, 0xed, 0xcf, 0x6d, 0x56, 0xd8, 0xcd, 0x8f,
    0x36, 0xe5, 0xa5, 0x4d, 0x58, 0xae, 0x61, 0x55, 0xed, 0xf4, 0x4e, 0x8c, 0x45, 0x4a, 0x67, 0xd1,
    0x65, 0xd4, 0xe6, 0x71, 0x55, 0x91, 0x41, 0x4d, 0x74, 0x84, 0x52, 0x7e, 0xe0, 0xb9, 0xca, 0x4c,
    0x6e, 0x62, 0x73, 0xf1, 0xa8, 0x5c, 0x97, 0x2f, 0x44, 0xb2, 0xed, 0x0e, 0xe4, 0x38, 0xc2, 0xd7,
    0xd0, 0x72, 0xc6, 0x33, 0x94, 0xd4, 0x6d, 0x86, 0xe2, 0xaa, 0x3e, 0xb9, 0x40, 0x70, 0xaf, 0xac,
    0x9d, 0x7a, 0x6b, 0xa0, 0x7b, 0x26, 0xe0, 0x32, 0xfe, 0xcf, 0xbe, 0xd4, 0xcf, 0x09, 0x26, 0x01,
    0xca, 0xa2, 0x33, 0xc7, 0x7b, 0xd9, 0xdb, 0xc4, 0x49, 0x07, 0xd4, 0x4e, 0xf5, 0x72, 0x0c, 0x57,
    0x95, 0xfc, 0x1b, 0xf3, 0x6e, 0xe3, 0x53, 0x03, 0x94, 0x3e, 0x03, 0x87, 0x7a, 0x08, 0x6d, 0xac,
    0x9b, 0x65, 0x6e, 0xe8, 0x64, 0xcd, 0x84, 0x76, 0x74, 0xe6, 0xa1, 0x02, 0x77, 0x92, 0x3e, 0xed,
    0x1b, 0xc6, 0xfc, 0x4a, 0x14, 0x75, 0x69, 0x2f, 0x7f, 0x67, 0x65, 0x8e, 0x9c, 0xff, 0xc9, 0x78,
    0x6c, 0x72, 0x68, 0xfa, 0x95, 0xc2, 0x25, 0x25, 0x83, 0xda, 0xa8, 0xfc, 0xe7, 0xf7, 0xd5, 0x7b,
    0x3b, 0xff, 0x0b, 0x9a, 0x40, 0x51, 0xe1, 0x21, 0x4e, 0xc2, 0xe8, 0x06, 0xc7, 0xf1, 0x90, 0xf3,
    0x9e, 0x9d, 0xe2, 0xae, 0x71, 0x6e, 0x81, 0x65, 0x11, 0xd1, 0x70, 0xb6, 0x04, 0xf7, 0xaa, 0xfb,
    0x39, 0x18, 0xe6, 0x11, 0x3a, 0x66, 0x3a, 0x4c, 0xff, 0xd1, 0x1b, 0xfc, 0x13, 0x84, 0x36, 0x88,
    0x8d, 0x98, 0x41, 0x89, 0x22, 0xde, 0xfa, 0x9c, 0x39, 0xdd, 0x0d, 0x0d, 0x63, 0x50, 0xbb, 0x23,
    0x71, 0xe4, 0x29, 0xfb, 0x56, 0x0a, 0x74, 0xfa, 0x7c, 0x10, 0x99, 0x5b, 0x1e, 0xdf, 0xab, 0xf1
};
static const uint32_t rsa_pubexp = 0x10001;

/** Mini-GMP memory allocation functions **/

static void *ios_gmp_alloc(size_t size)
{
    return IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, size);
}
static void *ios_gmp_realloc(void *buf, size_t old_size, size_t new_size)
{
    void *newbuf = IOS_HeapAlloc(CROSS_PROCESS_HEAP_ID, new_size);
    if (!newbuf)
        return NULL;

    // Copy the existing buffer into the new buffer.
    size_t to_copy = (old_size < new_size ? old_size : new_size);
    if (to_copy > 0) {
        memcpy(newbuf, buf, to_copy);
    }
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, buf);
    return newbuf;
}

static void ios_gmp_free(void *p, size_t unused_size)
{
    ((void)unused_size);
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, p);
}

/**
 * Encrypt data with our 2048-bit RSA key.
 * NOTE: This is NOT padded; this is *raw* encryption.
 * @param buf   [in/out] Data buffer for encryption. (must be 256 bytes)
 * @param size  [in] Size of buf (must be 256 bytes)
 * @return 0 on success; non-zero on error.
 */
int rsa2048_raw_encrypt(uint8_t* buf, size_t size)
{
    if (size != sizeof(rsa_pubkey))
        return -1;

    // Initialize gmp's memory allocation functions.
    mp_set_memory_functions(ios_gmp_alloc, ios_gmp_realloc, ios_gmp_free);

    // Initialize bignums.
    mpz_t n;	// public key
    mpz_t x;	// data to encrypt
    mpz_t f;	// result
    mpz_init(n);
    mpz_init(x);
    mpz_init(f);

    mpz_import(n, 1, 1, sizeof(rsa_pubkey), 1, 0, rsa_pubkey);
    mpz_import(x, 1, 1, size, 1, 0, buf);
    mpz_powm_ui(f, x, rsa_pubexp, n);

    mpz_clear(n);
    mpz_clear(x);

    // Get the encrypted data.
    mpz_export(buf, NULL, 1, size, 1, 0, f);
    mpz_clear(f);

    return 0;
}

/**
 * Encrypt a 128-bit AES key with our 2048-bit RSA key, using PKCS#1-style padding.
 * @param buf       [out] Encrypted AES key (must be 256 bytes)
 * @param buf_sz    [in] Size of buf (must be 256 bytes)
 * @param key       [in] 128-bit AES key (must be 16 bytes)
 * @param key_sz    [in] Size of key (must be 16 bytes)
 * @return 0 on success; non-zero on error.
 */
int rsa2048_encrypt_aes128_key(uint8_t* buf, size_t buf_sz, const uint8_t* key, size_t key_sz)
{
    if (buf_sz != sizeof(rsa_pubkey) || key_sz != AES_128_KEY_SIZE)
        return -1;

    // Initialize the buffer with random data.
    int res = IOSC_GenerateRand(buf, buf_sz);
    if (res != 0)
        return res;

    // Set up PKCS#1-style padding.
    // Reference: https://www.rfc-editor.org/rfc/rfc2313
    static const size_t key_offset = sizeof(rsa_pubkey) - AES_128_KEY_SIZE;
    buf[0] = 0x00;
    buf[1] = 0x02;
    buf[key_offset-1] = 0x00;

    // Copy in the AES key.
    memcpy(&buf[key_offset], key, AES_128_KEY_SIZE);

    // Encrypt the entire buffer using RSA.
    return rsa2048_raw_encrypt(buf, buf_sz);
}
