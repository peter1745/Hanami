#pragma once

#include "Kori/Core.hpp"
#include "WebEngine/Core/Core.hpp"

#include <print>

namespace Hanami::CSS {

    struct IdentToken
    {
        std::string value;
    };

    struct FunctionToken
    {
        std::string value;
    };

    struct AtKeywordToken {};

    struct HashToken
    {
        std::string value;
    };

    struct StringToken {};
    struct BadStringToken {};
    struct URLToken {};
    struct BadURLToken {};

    struct DelimToken
    {
        char value;
    };

    struct NumberToken {};
    struct PercentageToken {};
    struct DimensionToken {};
    struct WhitespaceToken {};
    struct CDOToken {};
    struct CDCToken {};
    struct ColonToken {};
    struct SemicolonToken {};
    struct CommaToken {};
    struct OpenSquareBracketToken {};
    struct CloseSquareBracketToken {};
    struct OpenParenthesesToken {};
    struct CloseParenthesesToken {};
    struct OpenCurlyBracketToken {};
    struct CloseCurlyBracketToken {};
    struct EOFToken {};

    using Token = std::variant<
        IdentToken,
        FunctionToken,
        AtKeywordToken,
        HashToken,
        StringToken,
        BadStringToken,
        URLToken,
        BadURLToken,
        DelimToken,
        NumberToken,
        PercentageToken,
        DimensionToken,
        WhitespaceToken,
        CDOToken,
        CDCToken,
        ColonToken,
        SemicolonToken,
        CommaToken,
        OpenSquareBracketToken,
        CloseSquareBracketToken,
        OpenParenthesesToken,
        CloseParenthesesToken,
        OpenCurlyBracketToken,
        CloseCurlyBracketToken,
        EOFToken
    >;

    inline void print_token(const Token& token)
    {
        std::visit(Kori::VariantOverloadSet {
            [](const IdentToken& token) { std::println("IdentToken({})", token.value); },
            [](const FunctionToken& token) { std::println("FunctionToken({})", token.value); },
            [](const AtKeywordToken&) { std::println("AtKeywordToken"); },
            [](const HashToken& token) { std::println("HashToken({})", token.value); },
            [](const StringToken&) { std::println("StringToken"); },
            [](const BadStringToken&) { std::println("BadStringToken"); },
            [](const URLToken&) { std::println("URLToken"); },
            [](const BadURLToken&) { std::println("BadURLToken"); },
            [](const DelimToken& token) { std::println("DelimToken({})", token.value); },
            [](const NumberToken&) { std::println("NumberToken"); },
            [](const PercentageToken&) { std::println("PercentageToken"); },
            [](const DimensionToken&) { std::println("DimensionToken"); },
            [](const WhitespaceToken&) { std::println("WhitespaceToken"); },
            [](const CDOToken&) { std::println("CDOToken"); },
            [](const CDCToken&) { std::println("CDCToken"); },
            [](const ColonToken&) { std::println("ColonToken"); },
            [](const SemicolonToken&) { std::println("SemicolonToken"); },
            [](const CommaToken&) { std::println("CommaToken"); },
            [](const OpenSquareBracketToken&) { std::println("OpenSquareBracketToken"); },
            [](const CloseSquareBracketToken&) { std::println("CloseSquareBracketToken"); },
            [](const OpenParenthesesToken&) { std::println("OpenParenthesesToken"); },
            [](const CloseParenthesesToken&) { std::println("CloseParenthesesToken"); },
            [](const OpenCurlyBracketToken&) { std::println("OpenCurlyBracketToken"); },
            [](const CloseCurlyBracketToken&) { std::println("CloseCurlyBracketToken"); },
            [](const EOFToken&) { std::println("EOFToken"); },
        }, token);
    }

    // TODO(Peter): Abstract some of these functions into a common interface between CSS::Tokenizer and HTML::Parser
    class Tokenizer
    {
    public:
        auto tokenize(std::string_view input) -> std::vector<Token>;

    private:
        auto consume_token() -> Token;
        auto consume_ident_like() -> Token;
        auto consume_ident_sequence() -> std::string;
        auto consume_next_character() noexcept -> char;

    private:
        std::string m_input_stream;
        size_t m_next_char_idx = 0;
        bool m_reached_eof = false;
    };

}
