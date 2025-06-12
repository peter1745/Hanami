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

    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    inline auto is_ascii_alpha_numeric(char c) -> bool
    {
        return (c >= '0' && c <= '9') || is_ascii_alpha(c);
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
