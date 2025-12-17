#include <iostream>
#include <string>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <vector>
#include <dpp/signature_verifier.h>

namespace dpp {

/**
 * @brief Converts a hex-encoded string to a byte vector.
 * @param hex The hex string to convert.
 * @return A vector containing the byte representation, or an empty vector on failure.
 */
std::vector<unsigned char> hex_to_bytes(const std::string& hex) {
    if (hex.length() % 2 != 0) {
	return {};
    }

    std::vector<unsigned char> bytes(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
	unsigned int byte;
	if (sscanf(hex.c_str() + i, "%2x", &byte) != 1) {
	    return {};
	}
	bytes[i / 2] = static_cast<unsigned char>(byte);
    }
    return bytes;
}

signature_verifier::signature_verifier() {
	OpenSSL_add_all_algorithms();
}

bool signature_verifier::verify_signature(const std::string& timestamp, const std::string& body, const std::string& signature_hex, const std::string& public_key_hex) {

	// Convert hex values to byte arrays
	std::vector<unsigned char> public_key = hex_to_bytes(public_key_hex);
	std::vector<unsigned char> signature = hex_to_bytes(signature_hex);

	if (public_key.size() != 32 || signature.size() != 64) {
	    return false;
	}

	// Create EVP_PKEY structure
	EVP_PKEY* evp_public_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, public_key.data(), public_key.size());
	if (!evp_public_key) {
	    return false;
	}

	// Create EVP_MD_CTX for signature verification
	EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
	if (!md_ctx) {
	    EVP_PKEY_free(evp_public_key);
	    return false;
	}

	// Prepare the message (concatenate timestamp and body)
	std::string message = timestamp + body;

	// Verify the signature
	bool valid = (EVP_DigestVerifyInit(md_ctx, nullptr, nullptr, nullptr, evp_public_key) == 1 && EVP_DigestVerify(md_ctx, signature.data(), signature.size(), reinterpret_cast<const unsigned char*>(message.data()), message.size()) == 1);

	// Clean up
	EVP_MD_CTX_free(md_ctx);
	EVP_PKEY_free(evp_public_key);

	return valid;
}

}

