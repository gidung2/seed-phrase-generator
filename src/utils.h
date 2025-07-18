#pragma once

// C++ 표준 헤더
#include <cstddef>      // size_t
#include <string>       // std::string
#include <vector>       // std::vector
#include <array>        // std::array

// OpenSSL forward declarations (구현부에서만 실제 include)
struct SHA256_CTX;
struct RIPEMD160_CTX;
struct EVP_MD_CTX;

namespace crypto_utils {

//==============================================================================
// Base58 인코딩
//==============================================================================
// 입력 데이터를 복사 없이 읽고, std::string으로 바로 반환
// RVO(return value optimization) 덕분에 불필요한 복사 없음.
std::string base58_encode(const std::vector<uint8_t>& input) noexcept;

//==============================================================================
// 해시 함수
//==============================================================================
// SHA-256: 고정 32바이트 반환, C-API 직접 호출로 메모리 재할당 최소화
std::array<uint8_t, 32> sha256(const uint8_t* data, size_t len) noexcept;
std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data) noexcept;

// RIPEMD-160: 고정 20바이트 반환
std::array<uint8_t, 20> ripemd160(const uint8_t* data, size_t len) noexcept;
std::array<uint8_t, 20> ripemd160(const std::vector<uint8_t>& data) noexcept;

// KECCAK-256: 32바이트 해시를 16진수 문자열로 반환
std::string keccak256(const std::string& input) noexcept;

//==============================================================================
// HMAC-SHA512
//==============================================================================
// 키와 메시지를 const-ref로 전달, 64바이트 고정 길이 반환
std::array<uint8_t, 64> hmac_sha512(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data) noexcept;

//==============================================================================
// Bech32 체크섬 · 인코딩
//==============================================================================
// 내부 연산은 int 벡터, 최종 주소는 std::string
std::vector<int> create_bech32_checksum(const std::vector<int>& data) noexcept;
std::string bech32_encode(const std::string& hrp,
                          const std::vector<uint8_t>& data) noexcept;

//==============================================================================
// BIP39 니모닉 → 시드
//==============================================================================
// PBKDF2-HMAC-SHA512 호출, 64바이트 반환
std::array<uint8_t, 64> mnemonic_to_seed(
    const std::string& mnemonic,
    const std::string& passphrase = "") noexcept;

//==============================================================================
// 유틸리티
//==============================================================================
// 바이트 배열 → 16진수 문자열 (디버그/로깅용)
std::string bytes_to_hex(const uint8_t* data, size_t len) noexcept;
std::string bytes_to_hex(const std::vector<uint8_t>& data) noexcept;

} // namespace crypto_utils
