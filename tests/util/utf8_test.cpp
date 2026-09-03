#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "util/utf8.h"

namespace {

void AppendCodePoint(std::string& text, std::uint32_t codepoint) {
    if (codepoint < 0x80) {
        text.push_back(static_cast<char>(codepoint));
        return;
    }
    const int count = codepoint < 0x800 ? 2 : codepoint < 0x10000 ? 3 : 4;
    text.push_back(static_cast<char>((0xff << (8 - count)) | (codepoint >> (6 * (count - 1)))));
    for (int index = count - 2; index >= 0; --index) {
        text.push_back(static_cast<char>(0x80 | ((codepoint >> (6 * index)) & 0x3f)));
    }
}

bool Check(std::string_view text, int expected, int line) {
    const int actual = Utf8CharacterCount(text);
    if (actual == expected) {
        return true;
    }
    std::cerr << "UTF-8 test line " << line << ": expected " << expected << ", got " << actual << '\n';
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        return 1;
    }
    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open " << argv[1] << '\n';
        return 1;
    }
    int cases = 0;
    int lineNumber = 0;
    for (std::string line; std::getline(input, line);) {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream fields(line);
        std::string boundary;
        std::string codepoint;
        std::string text;
        int expected = 0;
        while (fields >> boundary >> codepoint) {
            if (boundary == "\xc3\xb7") {
                ++expected;
            }
            AppendCodePoint(text, static_cast<std::uint32_t>(std::stoul(codepoint, nullptr, 16)));
            if (!Check(text, expected, lineNumber)) {
                return 1;
            }
        }
        if (!text.empty()) {
            ++cases;
        }
    }
    if (
        cases == 0 ||
        !Check({}, 0, __LINE__) ||
        !Check(std::string_view("a\0b", 3), 3, __LINE__) ||
        !Check("\x80", 1, __LINE__) ||
        !Check("\xc0\xaf", 2, __LINE__) ||
        !Check("\xe0\x80\xaf", 3, __LINE__) ||
        !Check("\xed\xa0\x80", 3, __LINE__) ||
        !Check("\xf0\x80\x80\xaf", 4, __LINE__) ||
        !Check("\xf4\x90\x80\x80", 4, __LINE__) ||
        !Check("\xf0\x9f\x91", 3, __LINE__) ||
        !Check("\xf0\x9f!", 3, __LINE__) ||
        !Check("a\xff\xcc\x81", 3, __LINE__)
    ) {
        return 1;
    }
    std::cout << "Passed " << cases << " Unicode grapheme cases and malformed UTF-8 checks.\n";
}
