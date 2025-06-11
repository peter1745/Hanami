#pragma once

#include <cctype>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Hanami {

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
