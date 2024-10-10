/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Contains macros from BoringSSL, Copyright (c) 2014, Google Inc.
 * Adapted from the public domain, estream code by D. Bernstein.
 *
 ************************************************************************************/

#include <openssl/evp.h>
#include <cstring>
#include <cstdint>
#include <dpp/exception.h>
#include "enabled.h"
#if _MSC_VER
	#include <stdlib.h>
#endif

/**
 * @brief ChaCha static constant
 * For some reason this is the ASCII string 'expand 32-byte k'.
 * It is specified by the standard, and cannot be changed.
 */
constexpr uint8_t CHACHA20_CONSTANT_SEED[16] = {'e', 'x', 'p', 'a', 'n', 'd', ' ', '3', '2', '-', 'b', 'y', 't', 'e', ' ', 'k' };

/**
 * XChaCha20-Poly1305 key size
 */
constexpr size_t KEY_SIZE = 32;

/**
 * @brief ChaCha20-Poly1305 nonce size
 */
constexpr size_t CHACHA_NONCE_SIZE = 12;

/**
 * @brief Rotates the bits of a 32-bit unsigned integer to the left by a specified number of positions.
 * From Google's BoringSSL
 * @param value value to shift
 * @param shift number of bits to shift left by
 * @return shifted value
 */
inline uint32_t rotl_u32(uint32_t value, int shift) {
#if _MSC_VER
	return _rotl(value, shift);
#else
	return (value << shift) | (value >> ((-shift) & 31));
#endif
}

/**
 * @brief Updates a, b, c, and d with a ChaCha20 quarter round.
 * This is black box voodoo from BoringSSL. Do not mess with it,
 * or try and optimise it. It is what it is!
 */
#define QUARTERROUND(a, b, c, d)		\
	x[a] += x[b];				\
	x[d] = rotl_u32(x[d] ^ x[a], 16);	\
	x[c] += x[d];				\
	x[b] = rotl_u32(x[b] ^ x[c], 12); 	\
	x[a] += x[b];				\
	x[d] = rotl_u32(x[d] ^ x[a], 8);  	\
	x[c] += x[d];				\
	x[b] = rotl_u32(x[b] ^ x[c], 7);

/**
 * @brief key derivation function that takes a 256-bit key and a 128-bit nonce, producing a 256-bit subkey.
 * 
 * This subkey is then used in the XChaCha20 algorithm to extend the nonce size to 192 bits.
 * This is taken from BoringSSL. Do not mess with it.
 * @param out output 32 byte subkey
 * @param key input 32 byte key
 * @param nonce input 16 byte nonce
 */
void hchacha20(unsigned char out[KEY_SIZE], const unsigned char key[KEY_SIZE], const unsigned char nonce[16]) {
	uint32_t x[16];
	std::memcpy(x, CHACHA20_CONSTANT_SEED, sizeof(CHACHA20_CONSTANT_SEED));
	std::memcpy(&x[4], key, KEY_SIZE);
	std::memcpy(&x[CHACHA_NONCE_SIZE], nonce, 16);
	for (size_t i = 0; i < 20; i += 2) {
		QUARTERROUND(0, 4, 8, 12)
		QUARTERROUND(1, 5, 9, 13)
		QUARTERROUND(2, 6, 10, 14)
		QUARTERROUND(3, 7, 11, 15)
		QUARTERROUND(0, 5, 10, 15)
		QUARTERROUND(1, 6, 11, 12)
		QUARTERROUND(2, 7, 8, 13)
		QUARTERROUND(3, 4, 9, 14)
	}
	std::memcpy(out, &x[0], sizeof(uint32_t) * 4);
	std::memcpy(&out[16], &x[CHACHA_NONCE_SIZE], sizeof(uint32_t) * 4);
}

int ssl_crypto_aead_xchacha20poly1305_ietf_encrypt(unsigned char *c, unsigned long long *clen, const unsigned char *m, unsigned long long mlen, const unsigned char *ad, unsigned long long adlen, const unsigned char *nsec, const unsigned char *npub, const unsigned char *k) {
	unsigned char subkey[KEY_SIZE];
	/* Regular ChaCha20-Poly1305 uses 12-byte nonce */
	unsigned char chacha_nonce[CHACHA_NONCE_SIZE] = {0};
	EVP_CIPHER_CTX *ctx = nullptr;
	int len = 0;
	int ciphertext_len = 0;

	try {
		/* Derive the subkey using HChaCha20 */
		hchacha20(subkey, k, npub);

		/* Use the last 8 bytes of the 24-byte XChaCha20 nonce */
		std::memcpy(chacha_nonce + 4, npub + ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES, 8);

		/* Initialize encryption context with ChaCha20-Poly1305 */
		ctx = EVP_CIPHER_CTX_new();
		if (!ctx || !EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr)) {
			throw dpp::encryption_exception("Error initializing encryption context");
		}

		/* Set key and nonce */
		if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, subkey, chacha_nonce)) {
			throw dpp::encryption_exception("Error setting key and nonce");
		}

		/* Set additional authenticated data (AAD) */
		if (!EVP_EncryptUpdate(ctx, nullptr, &len, ad, static_cast<int>(adlen))) {
			throw dpp::encryption_exception("Error setting additional authenticated data");
		}

		/* Encrypt the plaintext */
		if (!EVP_EncryptUpdate(ctx, c, &len, m, static_cast<int>(mlen))) {
			throw dpp::encryption_exception("Error during encryption");
		}
		ciphertext_len = len;

		if (!EVP_EncryptFinal_ex(ctx, c + len, &len)) {
			throw dpp::encryption_exception("Error finalizing encryption");
		}
		ciphertext_len += len;

		/* Get the authentication tag */
		if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES, c + ciphertext_len)) {
			throw dpp::encryption_exception("Error getting authentication tag");
		}

		/* Total ciphertext length (ciphertext + tag) */
		if (clen != nullptr) {
			*clen = ciphertext_len + ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES;
		}

	} catch (const dpp::encryption_exception& e) {
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	EVP_CIPHER_CTX_free(ctx);
	/* Safety */
	std::memset(subkey, 0, sizeof(subkey));
	return 0;
}

int ssl_crypto_aead_xchacha20poly1305_ietf_decrypt(unsigned char *m, unsigned long long *mlen, [[maybe_unused]] unsigned char *nsec, const unsigned char *c,  unsigned long long clen, const unsigned char *ad,	unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
	unsigned char subkey[KEY_SIZE];
	/* Regular ChaCha20-Poly1305 uses 12-byte nonce */
	unsigned char chacha_nonce[CHACHA_NONCE_SIZE] = {0};
	EVP_CIPHER_CTX *ctx = nullptr;
	int len = 0;
	int plaintext_len = 0;

	if (clen < ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES) {
		/* Ciphertext length must include at least the tag (16 bytes) */
		return -1;
	}

	try {
		/* Derive the subkey using HChaCha20 */
		hchacha20(subkey, k, npub);

		/* Use the last 8 bytes of the 24-byte XChaCha20 nonce */
		std::memcpy(chacha_nonce + 4, npub + ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES, 8);

		/* Initialize decryption context with ChaCha20-Poly1305 */
		ctx = EVP_CIPHER_CTX_new();
		if (!ctx || !EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr)) {
			throw dpp::decryption_exception("Error initializing decryption context");
		}

		/* Set key and nonce */
		if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, subkey, chacha_nonce)) {
			throw dpp::decryption_exception("Error setting key and nonce");
		}

		/* Set additional authenticated data (AAD) */
		if (!EVP_DecryptUpdate(ctx, nullptr, &len, ad, static_cast<int>(adlen))) {
			throw dpp::decryption_exception("Error setting additional authenticated data");
		}

		/* Decrypt the ciphertext (excluding the tag) */
		if (!EVP_DecryptUpdate(ctx, m, &len, c, static_cast<int>(clen - ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES))) {
			throw dpp::decryption_exception("Error during decryption");
		}
		plaintext_len = len;

		/* Set the expected tag */
		if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES, const_cast<unsigned char *>(c + clen - ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES))) {
			throw dpp::decryption_exception("Error setting authentication tag");
		}

		/* Check tag */
		int ret = EVP_DecryptFinal_ex(ctx, m + len, &len);
		if (ret > 0) {
			/* Tag is valid, finalize plaintext length */
			*mlen = plaintext_len + len;
		} else {
			throw dpp::decryption_exception("Authentication failed");
		}

	} catch (const dpp::decryption_exception& e) {
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	EVP_CIPHER_CTX_free(ctx);
	/* Safety */
	std::memset(subkey, 0, sizeof(subkey));

	return 0;
}
