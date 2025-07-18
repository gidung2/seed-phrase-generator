#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <array>

struct SHA256_CTX;
struct RIPEMD160_CTX;
struct EVP_MD_CTX;

namespace crypto_utils {

std::string base58_encode(const std::vector<uint8_t>& input) noexcept;

std::array<uint8_t, 32> sha256(const uint8_t* data, size_t len) noexcept;
std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data) noexcept;

std::array<uint8_t, 20> ripemd160(const uint8_t* data, size_t len) noexcept;
std::array<uint8_t, 20> ripemd160(const std::vector<uint8_t>& data) noexcept;

std::string keccak256(const std::string& input) noexcept;

std::array<uint8_t, 64> hmac_sha512(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data) noexcept;

std::vector<int> create_bech32_checksum(const std::vector<int>& data) noexcept;
std::string bech32_encode(const std::string& hrp,
                          const std::vector<uint8_t>& data) noexcept;

std::array<uint8_t, 64> mnemonic_to_seed(
    const std::string& mnemonic,
    const std::string& passphrase = "") noexcept;

std::string bytes_to_hex(const uint8_t* data, size_t len) noexcept;
std::string bytes_to_hex(const std::vector<uint8_t>& data) noexcept;

} 
