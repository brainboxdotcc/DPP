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
 ************************************************************************************/
#include <dpp/dpp.h>
#include <iostream>
#include <cstring>
#include <string>
#include <algorithm>

#include "../../mlspp/lib/bytes/include/bytes/bytes.h"
#include "../../src/dpp/dave/array_view.h"
#include "../../src/dpp/dave/openssl_aead_cipher.h"
#include "../../src/dpp/dave/key_ratchet.h"
#include "../../src/dpp/dave/common.h"

dpp::dave::EncryptionKey MakeStaticSenderKey(const std::string& userID);
dpp::dave::EncryptionKey MakeStaticSenderKey(uint64_t u64userID);

class StaticKeyRatchet : public dpp::dave::IKeyRatchet {
public:
	StaticKeyRatchet(const std::string& userId) noexcept;
	~StaticKeyRatchet() noexcept override = default;

	dpp::dave::EncryptionKey GetKey(dpp::dave::KeyGeneration generation) noexcept override;
	void DeleteKey(dpp::dave::KeyGeneration generation) noexcept override;

private:
	uint64_t u64userID_;
};

dpp::dave::EncryptionKey MakeStaticSenderKey(const std::string& userID)
{
	auto u64userID = strtoull(userID.c_str(), nullptr, 10);
	return MakeStaticSenderKey(u64userID);
}

dpp::dave::EncryptionKey MakeStaticSenderKey(uint64_t u64userID)
{
	static_assert(dpp::dave::kAesGcm128KeyBytes == 2 * sizeof(u64userID));
	dpp::dave::EncryptionKey senderKey(dpp::dave::kAesGcm128KeyBytes);
	const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(&u64userID);
	std::copy_n(bytePtr, sizeof(u64userID), senderKey.begin());
	std::copy_n(bytePtr, sizeof(u64userID), senderKey.begin() + sizeof(u64userID));
	return senderKey;
}

StaticKeyRatchet::StaticKeyRatchet(const std::string& userId) noexcept
	: u64userID_(strtoull(userId.c_str(), nullptr, 10)) {
}

dpp::dave::EncryptionKey StaticKeyRatchet::GetKey(dpp::dave::KeyGeneration generation) noexcept
{
	return MakeStaticSenderKey(u64userID_);
}

void StaticKeyRatchet::DeleteKey([[maybe_unused]] dpp::dave::KeyGeneration generation) noexcept {
}

std::string get_testdata_dir() {
	char *env_var = getenv("TEST_DATA_DIR");
	return (env_var ? env_var : "../../testdata/");
}

std::vector<uint8_t> load_test_audio() {
	std::vector<uint8_t> testaudio;
	std::string dir = get_testdata_dir();
	std::ifstream input (dir + "Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		size_t testaudio_size = input.tellg();
		testaudio.resize(testaudio_size);
		input.seekg(0, std::ios::beg);
		input.read((char*)testaudio.data(), testaudio_size);
		input.close();
	}
	else {
		std::cout << "ERROR: Can't load " + dir + "Robot.pcm\n";
		exit(1);
	}
	return testaudio;
}

/*#define EXPECT_TRUE(expr) if (!(expr)) { std::cout << "Failed\n"; exit(1); }
#define EXPECT_FALSE(expr) if (expr) { std::cout << "Failed\n"; exit(1); }

void encryptor_unit_test() {
	constexpr size_t PLAINTEXT_SIZE = 1024;
	auto plaintextBufferIn = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto additionalDataBuffer = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto plaintextBufferOut = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto ciphertextBuffer = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto nonceBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128NonceBytes, 0);
	auto tagBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128TruncatedTagBytes, 0);

	auto plaintextIn =
		dpp::dave::make_array_view<const uint8_t>(plaintextBufferIn.data(), plaintextBufferIn.size());
	auto additionalData =
		dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer.data(), additionalDataBuffer.size());
	auto plaintextOut =
		dpp::dave::make_array_view<uint8_t>(plaintextBufferOut.data(), plaintextBufferOut.size());
	auto ciphertextOut = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer.data(), ciphertextBuffer.size());
	auto ciphertextIn =
		dpp::dave::make_array_view<const uint8_t>(ciphertextBuffer.data(), ciphertextBuffer.size());
	auto nonce = dpp::dave::make_array_view<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());
	auto tagOut = dpp::dave::make_array_view<uint8_t>(tagBuffer.data(), tagBuffer.size());
	auto tagIn = dpp::dave::make_array_view<const uint8_t>(tagBuffer.data(), tagBuffer.size());

	dpp::dave::openssl_aead_cipher cryptor(MakeStaticSenderKey("12345678901234567890"));

	EXPECT_TRUE(cryptor.encrypt(ciphertextOut, plaintextIn, nonce, additionalData, tagOut));

	// The ciphertext should not be the same as the plaintext
	EXPECT_FALSE(memcmp(plaintextBufferIn.data(), ciphertextBuffer.data(), PLAINTEXT_SIZE) == 0);

	EXPECT_TRUE(cryptor.decrypt(plaintextOut, ciphertextIn, tagIn, nonce, additionalData));

	// The plaintext should be the same as the original plaintext
	EXPECT_TRUE(memcmp(plaintextBufferIn.data(), plaintextBufferOut.data(), PLAINTEXT_SIZE) == 0);

	plaintextBufferIn = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	additionalDataBuffer = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	plaintextBufferOut = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	ciphertextBuffer = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	nonceBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128NonceBytes, 0);
	tagBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128TruncatedTagBytes, 0);

	plaintextIn =
	dpp::dave::make_array_view<const uint8_t>(plaintextBufferIn.data(), plaintextBufferIn.size());
	auto additionalData1 =
	dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer.data(), additionalDataBuffer.size());
	plaintextOut =
		dpp::dave::make_array_view<uint8_t>(plaintextBufferOut.data(), plaintextBufferOut.size());
	ciphertextOut = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer.data(), ciphertextBuffer.size());
	ciphertextIn =
		dpp::dave::make_array_view<const uint8_t>(ciphertextBuffer.data(), ciphertextBuffer.size());
	nonce = dpp::dave::make_array_view<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());
	tagOut = dpp::dave::make_array_view<uint8_t>(tagBuffer.data(), tagBuffer.size());
	tagIn = dpp::dave::make_array_view<const uint8_t>(tagBuffer.data(), tagBuffer.size());

	dpp::dave::openssl_aead_cipher cryptor4(MakeStaticSenderKey("12345678901234567890"));

	EXPECT_TRUE(cryptor4.encrypt(ciphertextOut, plaintextIn, nonce, additionalData1, tagOut));

	// We modify the additional data before decryption
	additionalDataBuffer[0] = 1;

	EXPECT_FALSE(cryptor4.decrypt(plaintextOut, ciphertextIn, tagIn, nonce, additionalData1));

	auto plaintextBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto additionalDataBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto plaintextBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto additionalDataBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto ciphertextBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto ciphertextBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	nonceBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128NonceBytes, 0);
	tagBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128TruncatedTagBytes, 0);

	auto plaintext1 =
		dpp::dave::make_array_view<const uint8_t>(plaintextBuffer1.data(), plaintextBuffer1.size());
	additionalData1 =
		dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer1.data(), additionalDataBuffer1.size());
	auto plaintext2 =
		dpp::dave::make_array_view<const uint8_t>(plaintextBuffer2.data(), plaintextBuffer2.size());
	auto additionalData2 =
		dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer2.data(), additionalDataBuffer2.size());
	auto ciphertext1 = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer1.data(), ciphertextBuffer1.size());
	auto ciphertext2 = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer2.data(), ciphertextBuffer2.size());
	nonce = dpp::dave::make_array_view<const uint8_t>(nonceBuffer.data(), nonceBuffer.size());
	auto tag = dpp::dave::make_array_view<uint8_t>(tagBuffer.data(), tagBuffer.size());

	dpp::dave::openssl_aead_cipher cryptor1(MakeStaticSenderKey("12345678901234567890"));
	dpp::dave::openssl_aead_cipher cryptor2(MakeStaticSenderKey("09876543210987654321"));

	EXPECT_TRUE(cryptor1.encrypt(ciphertext1, plaintext1, nonce, additionalData1, tag));
	EXPECT_TRUE(cryptor2.encrypt(ciphertext2, plaintext2, nonce, additionalData2, tag));

	EXPECT_FALSE(memcmp(ciphertextBuffer1.data(), ciphertextBuffer2.data(), PLAINTEXT_SIZE) == 0);

	plaintextBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	additionalDataBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	plaintextBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	additionalDataBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	ciphertextBuffer1 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	ciphertextBuffer2 = std::vector<uint8_t>(PLAINTEXT_SIZE, 0);
	auto nonceBuffer1 = std::vector<uint8_t>(dpp::dave::kAesGcm128NonceBytes, 0);
	auto nonceBuffer2 = std::vector<uint8_t>(dpp::dave::kAesGcm128NonceBytes, 1);
	tagBuffer = std::vector<uint8_t>(dpp::dave::kAesGcm128TruncatedTagBytes, 0);

	plaintext1 =
		dpp::dave::make_array_view<const uint8_t>(plaintextBuffer1.data(), plaintextBuffer1.size());
	additionalData1 =
		dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer1.data(), additionalDataBuffer1.size());
	plaintext2 =
		dpp::dave::make_array_view<const uint8_t>(plaintextBuffer2.data(), plaintextBuffer2.size());
	additionalData2 =
		dpp::dave::make_array_view<const uint8_t>(additionalDataBuffer2.data(), additionalDataBuffer2.size());
	ciphertext1 = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer1.data(), ciphertextBuffer1.size());
	ciphertext2 = dpp::dave::make_array_view<uint8_t>(ciphertextBuffer2.data(), ciphertextBuffer2.size());
	auto nonce1 = dpp::dave::make_array_view<const uint8_t>(nonceBuffer1.data(), nonceBuffer1.size());
	auto nonce2 = dpp::dave::make_array_view<const uint8_t>(nonceBuffer2.data(), nonceBuffer2.size());
	tag = dpp::dave::make_array_view<uint8_t>(tagBuffer.data(), tagBuffer.size());

	dpp::dave::openssl_aead_cipher cryptor3(MakeStaticSenderKey("12345678901234567890"));

	EXPECT_TRUE(cryptor3.encrypt(ciphertext1, plaintext1, nonce1, additionalData1, tag));
	EXPECT_TRUE(cryptor3.encrypt(ciphertext2, plaintext2, nonce2, additionalData2, tag));

	EXPECT_FALSE(memcmp(ciphertextBuffer1.data(), ciphertextBuffer2.data(), PLAINTEXT_SIZE) == 0);
}*/

int main() {
	
	//encryptor_unit_test();
	
	using namespace std::chrono_literals;
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t == nullptr || getenv("TEST_GUILD_ID") == nullptr || getenv("TEST_VC_ID") == nullptr) {
		std::cerr << "Missing unit test environment. Set DPP_UNIT_TEST_TOKEN, TEST_GUILD_ID, and TEST_VC_ID\n";
		exit(1);
	}
	dpp::snowflake TEST_GUILD_ID(std::string(getenv("TEST_GUILD_ID")));
	dpp::snowflake TEST_VC_ID(std::string(getenv("TEST_VC_ID")));
	std::cout << "Test Guild ID: " << TEST_GUILD_ID << " Test VC ID: " << TEST_VC_ID << "\n\n";
	dpp::cluster dave_test(t, dpp::i_default_intents, 1, 0, 1, false, dpp::cache_policy_t{ dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none, dpp::cp_none });

	dave_test.on_log([&](const dpp::log_t& log) {
		std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
	});

	std::vector<uint8_t> testaudio = load_test_audio();

	dave_test.on_voice_ready([&](const dpp::voice_ready_t & event) {
		dave_test.log(dpp::ll_info, "Voice channel ready, sending audio...");
		dpp::discord_voice_client* v = event.voice_client;
		if (v && v->is_ready()) {
			v->send_audio_raw((uint16_t*)testaudio.data(), testaudio.size());
		}
	});


	dave_test.on_guild_create([&](const dpp::guild_create_t & event) {
		if (event.created->id == TEST_GUILD_ID) {
			dpp::discord_client* s = dave_test.get_shard(0);
			bool muted = false, deaf = false, enable_dave = true;
			s->connect_voice(TEST_GUILD_ID, TEST_VC_ID, muted, deaf, enable_dave);
		}
	});
	dave_test.start(false);
}
