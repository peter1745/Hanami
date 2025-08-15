#include "../../Test.hpp"

#include <WebEngine/CSS/Tokenizer.hpp>

DEFINE_SIMPLE_TEST({

    std::stringstream ss;
    std::ifstream stream("Tests/CSS/Tokenizer/css-tokenizer-basic.css");

    if (!stream)
    {
        TEST_FAIL("Failed reading CSS file. Does the file exist?");
    }

    ss << stream.rdbuf();

    auto tokens = Hanami::CSS::Tokenizer{}.run(ss.str());

    if (tokens.empty())
    {
        TEST_FAIL("Failed to parse any tokens");
    }

    int32_t indentation_level = 0;

    for (const auto& token : tokens)
    {
        if (std::holds_alternative<Hanami::CSS::LeftCurlyBracketToken>(token))
        {
            std::println("LeftCurlyBracketToken");
            ++indentation_level;
            continue;
        }

        if (std::holds_alternative<Hanami::CSS::RightCurlyBracketToken>(token))
        {
            std::println("RightCurlyBracketToken");
            --indentation_level;
            continue;
        }

        for (int32_t i = 0; i < indentation_level; ++i)
        {
            std::print("  ");
        }

        if (Hanami::CSS::token_is<Hanami::CSS::WhitespaceToken>(token))
        {
            std::print("\r");
        }
        else
        {
            Hanami::CSS::print_token(token);
        }
    }

    TEST_PASS();
});
