#pragma once

#include <print>
#include <algorithm>

#define TEST_FAIL(msg) status = -1; return
#define TEST_PASS() status = 0; return

namespace Hanami {

    struct ArgsList
    {
        std::span<char*> values;

        ArgsList() = default;

        ArgsList(int argc, char* argv[])
            : values(argv, argc)
        {
        }

        [[nodiscard]]
        auto has_arg(std::string_view arg) const -> bool
        {
            return std::ranges::any_of(values, [arg](const char* value) { return arg == value; });
        }
    };

}

#define INCLUDE_ARGS_LIST const auto args = ::Hanami::ArgsList{ argc - 1, argv + 1 };

#define DEFINE_SIMPLE_HTML_TEST(file, test, ...)\
    int main(int argc, char* argv[])\
    {\
        (void)argc;\
        (void)argv;\
        __VA_ARGS__\
        const auto* doc = Hanami::HTML::Parser::parse_from_file(file);\
        if (!doc) { return -1; }\
        int status = -1;\
        [&] test();\
        delete doc;\
        return status;\
    }

#define DEFINE_SIMPLE_TEST(test, ...)\
    int main(int argc, char* argv[])\
    {\
        (void)argc;\
        (void)argv;\
        __VA_ARGS__\
        int status = -1;\
        [&] test();\
        return status;\
    }
