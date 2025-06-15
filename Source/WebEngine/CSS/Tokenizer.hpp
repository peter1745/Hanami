#pragma once

#include "WebEngine/Core/Core.hpp"

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

    // TODO(Peter): Abstract some of these functions into a common interface between CSS::Tokenizer and HTML::Parser
    class Tokenizer
    {
    public:
        void tokenize(std::string_view input);

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
