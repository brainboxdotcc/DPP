#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <bytes/bytes.h>
#include <mls/crypto.h>
#include "parameters.h"
#include "version.h"

namespace mlspp {
struct SignaturePrivateKey;
};

namespace dpp::dave::mls {

using KeyPairContextType = const char *;

std::shared_ptr<::mlspp::SignaturePrivateKey> GetPersistedKeyPair(KeyPairContextType ctx,
                                                                  const std::string& sessionID,
                                                                  ProtocolVersion version);

struct KeyAndSelfSignature {
    std::vector<uint8_t> key;
    std::vector<uint8_t> signature;
};

KeyAndSelfSignature GetPersistedPublicKey(KeyPairContextType ctx,
                                          const std::string& sessionID,
                                          SignatureVersion version);

bool DeletePersistedKeyPair(KeyPairContextType ctx,
                            const std::string& sessionID,
                            SignatureVersion version);

constexpr unsigned KeyVersion = 1;

} // namespace dpp::dave::mls



namespace dpp {
	namespace dave {
		namespace mls {
			namespace detail {
				std::shared_ptr<::mlspp::SignaturePrivateKey> GetGenericPersistedKeyPair(KeyPairContextType ctx, const std::string& id, ::mlspp::CipherSuite suite);
				bool DeleteGenericPersistedKeyPair(KeyPairContextType ctx, const std::string& id);
			}
		}
	}
}