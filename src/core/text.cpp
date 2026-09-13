#include "vpp/core/text.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace vietvm::core {

namespace {

std::size_t utf8SequenceLength(unsigned char lead) {
    if ((lead & 0x80u) == 0) return 1;
    if ((lead & 0xe0u) == 0xc0u) return 2;
    if ((lead & 0xf0u) == 0xe0u) return 3;
    if ((lead & 0xf8u) == 0xf0u) return 4;
    return 1;
}

bool validUtf8Unit(const std::string &value, std::size_t offset, std::size_t length) {
    if (length == 1) return true;
    if (offset + length > value.size()) return false;
    for (std::size_t i = 1; i < length; ++i) {
        const unsigned char byte = static_cast<unsigned char>(value[offset + i]);
        if ((byte & 0xc0u) != 0x80u) return false;
    }
    return true;
}

} // namespace

std::string trim(const std::string &value) {
    const size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string toLowerAscii(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (unsigned char c : value) {
        if (c >= static_cast<unsigned char>('A') && c <= static_cast<unsigned char>('Z')) {
            result.push_back(static_cast<char>(c + ('a' - 'A')));
        } else {
            result.push_back(static_cast<char>(c));
        }
    }
    return result;
}

std::string toUpperAscii(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (unsigned char c : value) {
        if (c >= static_cast<unsigned char>('a') && c <= static_cast<unsigned char>('z')) {
            result.push_back(static_cast<char>(c - ('a' - 'A')));
        } else {
            result.push_back(static_cast<char>(c));
        }
    }
    return result;
}

std::vector<std::string> splitAsciiWords(const std::string &value) {
    std::vector<std::string> words;
    std::string current;
    for (unsigned char byte : value) {
        if (std::isspace(byte)) {
            if (!current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
        } else {
            current.push_back(static_cast<char>(byte));
        }
    }
    if (!current.empty()) words.push_back(std::move(current));
    return words;
}

std::string joinWithSpaces(const std::vector<std::string> &words) {
    std::ostringstream output;
    for (std::size_t index = 0; index < words.size(); ++index) {
        if (index != 0) output << ' ';
        output << words[index];
    }
    return output.str();
}

std::size_t utf8CodePointCount(const std::string &value) {
    std::size_t count = 0;
    for (std::size_t offset = 0; offset < value.size();) {
        std::size_t length = utf8SequenceLength(static_cast<unsigned char>(value[offset]));
        if (!validUtf8Unit(value, offset, length)) length = 1;
        offset += length;
        ++count;
    }
    return count;
}

std::string reverseUtf8CodePoints(const std::string &value) {
    std::vector<std::string> units;
    units.reserve(value.size());
    for (std::size_t offset = 0; offset < value.size();) {
        std::size_t length = utf8SequenceLength(static_cast<unsigned char>(value[offset]));
        if (!validUtf8Unit(value, offset, length)) length = 1;
        units.emplace_back(value.substr(offset, length));
        offset += length;
    }
    std::reverse(units.begin(), units.end());
    std::string result;
    result.reserve(value.size());
    for (const std::string &unit : units) result += unit;
    return result;
}

std::size_t countSubstring(const std::string &value, const std::string &needle) {
    if (needle.empty()) return 0;
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = value.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

std::string replaceAll(const std::string &value,
                       const std::string &from,
                       const std::string &to) {
    if (from.empty()) return value;
    std::string result = value;
    std::size_t position = 0;
    while ((position = result.find(from, position)) != std::string::npos) {
        result.replace(position, from.size(), to);
        position += to.size();
    }
    return result;
}

std::string longestAsciiWord(const std::string &value) {
    const std::vector<std::string> words = splitAsciiWords(value);
    std::string longest;
    for (const std::string &word : words) {
        if (utf8CodePointCount(word) > utf8CodePointCount(longest)) longest = word;
    }
    return longest;
}

std::string titleAsciiWords(const std::string &value) {
    std::vector<std::string> words = splitAsciiWords(value);
    for (std::string &word : words) {
        word = toLowerAscii(word);
        if (!word.empty()) {
            word.replace(0, 1, toUpperAscii(word.substr(0, 1)));
        }
    }
    return joinWithSpaces(words);
}

bool isAsciiCaseInsensitivePalindrome(const std::string &value) {
    const std::string normalized = toLowerAscii(value);
    return normalized == reverseUtf8CodePoints(normalized);
}

bool areAsciiAnagrams(const std::string &left, const std::string &right) {
    auto normalize = [](const std::string &value) {
        std::string normalized;
        for (unsigned char byte : toLowerAscii(value)) {
            if (!std::isspace(byte)) normalized.push_back(static_cast<char>(byte));
        }
        std::sort(normalized.begin(), normalized.end());
        return normalized;
    };
    return normalize(left) == normalize(right);
}

std::string caesarAscii(const std::string &value, int shift) {
    const int normalized = ((shift % 26) + 26) % 26;
    std::string result = value;
    for (char &byte : result) {
        if (byte >= 'a' && byte <= 'z') {
            byte = static_cast<char>('a' + (byte - 'a' + normalized) % 26);
        } else if (byte >= 'A' && byte <= 'Z') {
            byte = static_cast<char>('A' + (byte - 'A' + normalized) % 26);
        }
    }
    return result;
}

} // namespace vietvm::core
