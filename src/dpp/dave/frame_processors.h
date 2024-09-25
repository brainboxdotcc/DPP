#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "common.h"
#include "array_view.h"

namespace discord {
namespace dave {

struct Range {
    size_t offset;
    size_t size;
};
using Ranges = std::vector<Range>;

uint8_t UnencryptedRangesSize(const Ranges& unencryptedRanges);
uint8_t SerializeUnencryptedRanges(const Ranges& unencryptedRanges,
                                   uint8_t* buffer,
                                   size_t bufferSize);
uint8_t DeserializeUnencryptedRanges(const uint8_t*& buffer,
                                     const size_t bufferSize,
                                     Ranges& unencryptedRanges);
bool ValidateUnencryptedRanges(const Ranges& unencryptedRanges, size_t frameSize);

class InboundFrameProcessor {
public:
    void ParseFrame(ArrayView<const uint8_t> frame);
    size_t ReconstructFrame(ArrayView<uint8_t> frame) const;

    bool IsEncrypted() const { return isEncrypted_; }
    size_t Size() const { return originalSize_; }
    void Clear();

    ArrayView<const uint8_t> GetTag() const { return tag_; }
    TruncatedSyncNonce GetTruncatedNonce() const { return truncatedNonce_; }
    ArrayView<const uint8_t> GetAuthenticatedData() const
    {
        return MakeArrayView(authenticated_.data(), authenticated_.size());
    }
    ArrayView<const uint8_t> GetCiphertext() const
    {
        return MakeArrayView(ciphertext_.data(), ciphertext_.size());
    }
    ArrayView<uint8_t> GetPlaintext() { return MakeArrayView(plaintext_); }

private:
    void AddAuthenticatedBytes(const uint8_t* data, size_t size);
    void AddCiphertextBytes(const uint8_t* data, size_t size);

    bool isEncrypted_{false};
    size_t originalSize_{0};
    ArrayView<const uint8_t> tag_;
    TruncatedSyncNonce truncatedNonce_;
    Ranges unencryptedRanges_;
    std::vector<uint8_t> authenticated_;
    std::vector<uint8_t> ciphertext_;
    std::vector<uint8_t> plaintext_;
};

class OutboundFrameProcessor {
public:
    void ProcessFrame(ArrayView<const uint8_t> frame, Codec codec);
    size_t ReconstructFrame(ArrayView<uint8_t> frame);

    Codec GetCodec() const { return codec_; }
    const std::vector<uint8_t>& GetUnencryptedBytes() const { return unencryptedBytes_; }
    const std::vector<uint8_t>& GetEncryptedBytes() const { return encryptedBytes_; }
    std::vector<uint8_t>& GetCiphertextBytes() { return ciphertextBytes_; }
    const Ranges& GetUnencryptedRanges() const { return unencryptedRanges_; }

    void Reset();
    void AddUnencryptedBytes(const uint8_t* bytes, size_t size);
    void AddEncryptedBytes(const uint8_t* bytes, size_t size);

private:
    Codec codec_{Codec::Unknown};
    size_t frameIndex_{0};
    std::vector<uint8_t> unencryptedBytes_;
    std::vector<uint8_t> encryptedBytes_;
    std::vector<uint8_t> ciphertextBytes_;
    Ranges unencryptedRanges_;
};

} // namespace dave
} // namespace discord
