#include "utils.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/hmac.h>
#include <cstring>

//------------------------------------------------------------------------------
// 상수 정의
//------------------------------------------------------------------------------
static constexpr char BASE58_ALPHABET[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static constexpr char BECH32_ALPHABET[] =
    "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static constexpr char HEX_TABLE[] = "0123456789abcdef";

//------------------------------------------------------------------------------
// HEX 변환 (버퍼 할당 한 번 + 테이블 룩업)
//------------------------------------------------------------------------------
inline std::string bytes_to_hex(const uint8_t* data, size_t len) noexcept {
    std::string out;
    out.resize(len * 2);

    for (size_t i = 0, j = 0; i < len; ++i) {
        uint8_t b = data[i];
        out[j++] = HEX_TABLE[b >> 4];
        out[j++] = HEX_TABLE[b & 0x0F];
    }
    return out;
}

//------------------------------------------------------------------------------
// Base58 인코딩 (Big-Integer 스타일, 사전 할당 + in-place 계산)
//------------------------------------------------------------------------------
std::string base58_encode(const std::vector<uint8_t>& input) noexcept {
    if (input.empty()) return {};

    // 최대 자리수 계산: log(256^n)/log(58) ≒ n * 1.365, 안전하게 +1
    size_t maxDigits = input.size() * 138 / 100 + 1;
    std::vector<uint32_t> digits(maxDigits);
    size_t digitLen = 1;

    for (uint8_t byte : input) {
        uint32_t carry = byte;
        for (size_t i = 0; i < digitLen; ++i) {
            uint64_t val = uint64_t(digits[i]) * 256 + carry;
            digits[i] = uint32_t(val % 58);
            carry = uint32_t(val / 58);
        }
        while (carry > 0) {
            digits[digitLen++] = carry % 58;
            carry /= 58;
        }
    }

    // leading-zero bytes → ‘1’ 문자로
    size_t zeroCount = 0;
    while (zeroCount < input.size() && input[zeroCount] == 0) {
        ++zeroCount;
    }

    std::string result;
    result.reserve(zeroCount + digitLen);
    result.append(zeroCount, BASE58_ALPHABET[0]);
    for (size_t i = 0; i < digitLen; ++i) {
        result.push_back(BASE58_ALPHABET[digits[digitLen - 1 - i]]);
    }
    return result;
}

//------------------------------------------------------------------------------
// SHA-256, RIPEMD-160
//------------------------------------------------------------------------------
std::array<uint8_t, SHA256_DIGEST_LENGTH> sha256(const std::vector<uint8_t>& data) noexcept {
    std::array<uint8_t, SHA256_DIGEST_LENGTH> out;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(out.data(), &ctx);
    return out;
}

std::array<uint8_t, RIPEMD160_DIGEST_LENGTH> ripemd160(const std::vector<uint8_t>& data) noexcept {
    std::array<uint8_t, RIPEMD160_DIGEST_LENGTH> out;
    RIPEMD160_CTX ctx;
    RIPEMD160_Init(&ctx);
    RIPEMD160_Update(&ctx, data.data(), data.size());
    RIPEMD160_Final(out.data(), &ctx);
    return out;
}

//------------------------------------------------------------------------------
// KECCAK-256 (SHA3-256)
 // OpenSSL 1.1.1+ 필요
std::string keccak256(const std::string& input) noexcept {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, hash, nullptr);
    EVP_MD_CTX_free(ctx);
    return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

//------------------------------------------------------------------------------
// HMAC-SHA512
//------------------------------------------------------------------------------
std::array<uint8_t, SHA512_DIGEST_LENGTH> hmac_sha512(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data) noexcept
{
    std::array<uint8_t, SHA512_DIGEST_LENGTH> out;
    unsigned int len = 0;
    HMAC(EVP_sha512(),
         key.data(), int(key.size()),
         data.data(), data.size(),
         out.data(), &len);
    return out;
}

//------------------------------------------------------------------------------
// 비트 변환 (generic convertBits 함수)
//------------------------------------------------------------------------------
static std::vector<int> convert_bits(const std::vector<uint8_t>& in,
                                     int fromBits, int toBits, bool pad) noexcept
{
    uint32_t acc = 0;
    int bits = 0;
    const uint32_t maxv = (1u << toBits) - 1;
    std::vector<int> out;
    for (uint8_t v : in) {
        acc = (acc << fromBits) | v;
        bits += fromBits;
        while (bits >= toBits) {
            bits -= toBits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad && bits) {
        out.push_back((acc << (toBits - bits)) & maxv);
    }
    return out;
}

//------------------------------------------------------------------------------
// Bech32 체크섬 (모듈화) + 인코딩
//------------------------------------------------------------------------------
static uint32_t bech32_polymod(const std::vector<int>& v) noexcept {
    static constexpr uint32_t GENERATORS[5] = {
        0x3b6a57b2, 0x26508e6d, 0x1ea119fa,
        0x3d4233dd, 0x2a1462b3
    };
    uint32_t chk = 1;
    for (int x : v) {
        uint8_t top = chk >> 25;
        chk = ((chk & 0x1ffffff) << 5) ^ x;
        for (int i = 0; i < 5; ++i) {
            if (top & (1 << i)) chk ^= GENERATORS[i];
        }
    }
    return chk;
}

static std::vector<int> create_bech32_checksum(const std::vector<int>& data) noexcept {
    std::vector<int> v = data;
    v.insert(v.end(), {0,0,0,0,0,0});
    uint32_t mod = bech32_polymod(v) ^ 1;
    std::vector<int> ret(6);
    for (int i = 0; i < 6; ++i) {
        ret[i] = (mod >> (5 * (5 - i))) & 0x1f;
    }
    return ret;
}

std::string bech32_encode(const std::string& hrp,
                          const std::vector<uint8_t>& data) noexcept
{
    // HRP 확장
    std::vector<int> hrp_exp;
    hrp_exp.reserve(hrp.size() * 2 + 1);
    for (char c : hrp) {
        hrp_exp.push_back((c >> 5) & 0x07);
    }
    hrp_exp.push_back(0);
    for (char c : hrp) {
        hrp_exp.push_back(c & 0x1f);
    }

    // 데이터 변환
    auto conv = convert_bits(data, 8, 5, true);

    // 체크섬
    std::vector<int> all = hrp_exp;
    all.insert(all.end(), conv.begin(), conv.end());
    auto chk = create_bech32_checksum(all);

    // 최종 문자열 빌드
    std::string out;
    out.reserve(hrp.size() + 1 + conv.size() + chk.size());
    out += hrp;
    out += '1';
    for (int v : conv) out += BECH32_ALPHABET[v];
    for (int v : chk) out += BECH32_ALPHABET[v];
    return out;
}

//------------------------------------------------------------------------------
// BIP39 니모닉 → 시드 (PBKDF2-HMAC-SHA512)
//------------------------------------------------------------------------------
std::vector<uint8_t> mnemonic_to_seed(const std::string& mnemonic,
                                      const std::string& passphrase) noexcept
{
    std::string salt = "mnemonic" + passphrase;
    std::vector<uint8_t> seed(64);
    PKCS5_PBKDF2_HMAC(mnemonic.data(), int(mnemonic.size()),
                      reinterpret_cast<const uint8_t*>(salt.data()), int(salt.size()),
                      2048, EVP_sha512(),
                      int(seed.size()), seed.data());
    return seed;
}

//------------------------------------------------------------------------------
// bytearray2hex (기존 호환용)
std::string bytearray2hex(const std::vector<uint8_t>& arr) noexcept {
    return bytes_to_hex(arr.data(), arr.size());
}
