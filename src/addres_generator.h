#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <span>

// 워드리스트 로드: 파일 이름에 string_view 사용
std::vector<std::string> load_wordlist(std::string_view filename);

// 문자열 분할: 원본 복사 없이 string_view 조각 반환
std::vector<std::string_view> split_words(std::string_view str, char delim = ' ');

// 문자열 합치기: span + string_view 로 경량화
std::string join(std::span<const std::string_view> parts,
                 std::string_view delim = " ");

// 브루트포스 시드 문구 생성
// - pattern: "_" 가 들어간 string_view 배열
// - wordlist: 원본 단어 리스트 (const ref)
// - missing_indices: pattern에서 채워야 할 위치
// - target_address: 비교할 주소
// - depth, count: 재귀 깊이, 시도 횟수 (count는 반드시 외부에서 초기화)
wallet brute_force_seed_phrase(
    std::span<std::string_view> pattern,
    std::span<const std::string>    wordlist,
    std::span<const int>            missing_indices,
    std::string_view                target_address,
    size_t                          depth,
    size_t&                         count) noexcept;

// mnemonic 하나로 바로 지갑 찾기 (brute_force 내부 호출)
wallet brute_find_wallet(std::string_view mnemonic,
                         std::string_view target_address) noexcept;
