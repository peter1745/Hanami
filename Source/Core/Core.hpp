#pragma once

#include <span>
#include <string>
#include <vector>
#include <cctype>
#include <memory>
#include <variant>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <functional>
#include <string_view>
#include <unordered_map>

using namespace std::literals;

#if defined(HANAMI_PLATFORM_LINUX)
    #include <sys/signal.h>
    #define HANAMI_TRAP() raise(SIGTRAP)
#elif defined(HANAMI_PLATFORM_LINUX)
    #define HANAMI_TRAP() __debugbreak()
#endif

namespace Hanami {

    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    inline auto is_ascii_upper_alpha(char c) -> bool
    {
        return c >= 'A' && c <= 'Z';
    }

    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    inline auto is_ascii_lower_alpha(char c) -> bool
    {
        return c >= 'a' && c <= 'z';
    }

    // https://infra.spec.whatwg.org/#ascii-alpha
    inline auto is_ascii_alpha(char c) -> bool
    {
        return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
    }

    // https://infra.spec.whatwg.org/#ascii-digit
    inline auto is_ascii_digit(char c) -> bool
    {
        return c >= '0' && c <= '9';
    }

    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    inline auto is_ascii_alpha_numeric(char c) -> bool
    {
        return is_ascii_digit(c) || is_ascii_alpha(c);
    }

    // https://infra.spec.whatwg.org/#surrogate
    inline auto is_unicode_surrogate(uint32_t codepoint) -> bool
    {
        return (codepoint >= 0xD800 && codepoint <= 0xDBFF) || // leading surrogate
               (codepoint >= 0xDC00 && codepoint <= 0xDFFF);   // trailing surrogate
    }

    inline auto is_unicode_c0_control(uint32_t codepoint) -> bool
    {
        return /* codepoint >= 0x000 && */ codepoint <= 0x001F;
    }

    inline auto is_unicode_control(uint32_t codepoint) -> bool
    {
        return is_unicode_c0_control(codepoint) || (codepoint >= 0x007F && codepoint <= 0x009F);
    }

    // https://infra.spec.whatwg.org/#noncharacter
    inline auto is_unicode_noncharacter(uint32_t codepoint)
    {
        return (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) ||
                codepoint == 0xFFFE   ||
                codepoint == 0xFFFF   ||
                codepoint == 0x1FFFE  ||
                codepoint == 0x1FFFF  ||
                codepoint == 0x2FFFE  ||
                codepoint == 0x2FFFF  ||
                codepoint == 0x3FFFE  ||
                codepoint == 0x3FFFF  ||
                codepoint == 0x4FFFE  ||
                codepoint == 0x4FFFF  ||
                codepoint == 0x5FFFE  ||
                codepoint == 0x5FFFF  ||
                codepoint == 0x6FFFE  ||
                codepoint == 0x6FFFF  ||
                codepoint == 0x7FFFE  ||
                codepoint == 0x7FFFF  ||
                codepoint == 0x8FFFE  ||
                codepoint == 0x8FFFF  ||
                codepoint == 0x9FFFE  ||
                codepoint == 0x9FFFF  ||
                codepoint == 0xAFFFE  ||
                codepoint == 0xAFFFF  ||
                codepoint == 0xBFFFE  ||
                codepoint == 0xBFFFF  ||
                codepoint == 0xCFFFE  ||
                codepoint == 0xCFFFF  ||
                codepoint == 0xDFFFE  ||
                codepoint == 0xDFFFF  ||
                codepoint == 0xEFFFE  ||
                codepoint == 0xEFFFF  ||
                codepoint == 0xFFFFE  ||
                codepoint == 0xFFFFF  ||
                codepoint == 0x10FFFE ||
                codepoint == 0x10FFFF;
    }

    inline auto equals_case_insensitive(std::string_view a, std::string_view b) -> bool
    {
        if (a.length() != b.length())
        {
            return false;
        }

        for (size_t i = 0; i < a.length(); ++i)
        {
            if (std::tolower(a[i]) != std::tolower(b[i]))
            {
                return false;
            }
        }

        return true;
    }

}
