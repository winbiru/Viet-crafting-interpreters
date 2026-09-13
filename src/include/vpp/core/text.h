#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vietvm::core {

std::string trim(const std::string &value);

// Lowercase ASCII bytes while preserving UTF-8 bytes unchanged.
std::string toLowerAscii(const std::string &value);

// Uppercase ASCII bytes while preserving UTF-8 bytes unchanged.
std::string toUpperAscii(const std::string &value);

// Tokenize and join text using ASCII/locale-independent whitespace.  The
// functions intentionally preserve UTF-8 bytes and are shared by string and
// file native APIs.
std::vector<std::string> splitAsciiWords(const std::string &value);
std::string joinWithSpaces(const std::vector<std::string> &words);

// UTF-8 helpers used by user-facing string APIs. Invalid byte sequences are
// preserved as single-byte units so library calls remain total on arbitrary
// file/network text instead of throwing during basic length/reverse operations.
std::size_t utf8CodePointCount(const std::string &value);
std::string reverseUtf8CodePoints(const std::string &value);

// Shared string algorithms used by the native stdlib. Case conversion remains
// ASCII-only for now, while length/reverse and longest-word sizing use UTF-8
// code-point boundaries.
std::size_t countSubstring(const std::string &value, const std::string &needle);
std::string replaceAll(const std::string &value,
                       const std::string &from,
                       const std::string &to);
std::string longestAsciiWord(const std::string &value);
std::string titleAsciiWords(const std::string &value);
bool isAsciiCaseInsensitivePalindrome(const std::string &value);
bool areAsciiAnagrams(const std::string &left, const std::string &right);

// Rotate ASCII Latin letters by `shift`; other bytes are preserved.
std::string caesarAscii(const std::string &value, int shift);

} // namespace vietvm::core
