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

#pragma once

#include <stddef.h>
#include <stdint.h>

#define RSA2048_BUF_SIZE (2048/8)
#define AES_128_KEY_SIZE 16

/**
 * Encrypt data with our 2048-bit RSA key.
 * NOTE: This is NOT padded; this is *raw* encryption.
 * @param buf   [in/out] Data buffer for encryption. (must be 256 bytes)
 * @param size  [in] Size of buf (must be 256 bytes)
 */
int rsa2048_raw_encrypt(uint8_t* buf, size_t size);

/**
 * Encrypt a 128-bit AES key with our 2048-bit RSA key, using PKCS#1-style padding.
 * @param buf       [out] Encrypted AES key (must be 256 bytes)
 * @param buf_sz    [in] Size of buf (must be 256 bytes)
 * @param key       [in] 128-bit AES key (must be 16 bytes)
 * @param key_sz    [in] Size of key (must be 16 bytes)
 * @return 0 on success; non-zero on error.
 */
int rsa2048_encrypt_aes128_key(uint8_t* buf, size_t buf_sz, const uint8_t* key, size_t key_sz);
