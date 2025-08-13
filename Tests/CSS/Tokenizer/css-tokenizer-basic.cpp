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

    for (const auto& token : tokens)
    {
        std::println("Token: {}", Hanami::CSS::token_name(token));
        
        if (std::holds_alternative<Hanami::CSS::DelimToken>(token))
        {
            std::println("\tValue: {}", std::get<Hanami::CSS::DelimToken>(token).value);
        }
    }

    TEST_PASS();
});
