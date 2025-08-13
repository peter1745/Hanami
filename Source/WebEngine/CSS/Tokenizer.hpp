#pragma once

#include "WebEngine/Core/Core.hpp"
#include "Kori/Core.hpp"

namespace Hanami::CSS {

    // https://www.w3.org/TR/css-syntax-3/#tokenization

    struct IdentToken     { std::string value; };
    struct FunctionToken  { std::string value; };
    struct AtKeywordToken { std::string value; };

    struct HashToken
    {
        enum class Type { ID, Unrestricted };
        std::string value;
        Type type = Type::Unrestricted;
    };

    struct StringToken { std::string value; };
    struct BadStringToken {};

    struct URLToken { std::string value; };
    struct BadURLToken {};

    struct DelimToken { char value; };

    enum class NumericType { Integer, Number };
    struct NumberToken
    {
        double value;
        NumericType type = NumericType::Integer;
    };

    struct PercentageToken { double value; };

    struct DimensionToken
    {
        double value;
        NumericType type = NumericType::Integer;
        std::string unit;
    };

    struct WhitespaceToken {};
    struct CDOToken {};
    struct CDCToken {};
    struct ColonToken {};
    struct SemicolonToken {};
    struct CommaToken {};
    struct LeftSquareBracketToken {};
    struct RightSquareBracketToken {};
    struct LeftParenthesesToken {};
    struct RightParenthesesToken {};
    struct LeftCurlyBracketToken {};
    struct RightCurlyBracketToken {};
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
        LeftSquareBracketToken,
        RightSquareBracketToken,
        LeftParenthesesToken,
        RightParenthesesToken,
        LeftCurlyBracketToken,
        RightCurlyBracketToken,
        EOFToken
    >;

    [[nodiscard]]
    inline auto token_name(const Token& token) -> std::string_view
    {
        auto name = std::string_view{};
        
        std::visit(Kori::VariantOverloadSet {
            [&](const IdentToken&) { name = "IdentToken"; },
            [&](const FunctionToken&) { name = "FunctionToken"; },
            [&](const AtKeywordToken&) { name = "AtKeywordToken"; },
            [&](const HashToken&) { name = "HashToken"; },
            [&](const StringToken&) { name = "StringToken"; },
            [&](const BadStringToken&) { name = "BadStringToken"; },
            [&](const URLToken&) { name = "URLToken"; },
            [&](const BadURLToken&) { name = "BadURLToken"; },
            [&](const DelimToken&) { name = "DelimToken"; },
            [&](const NumberToken&) { name = "NumberToken"; },
            [&](const PercentageToken&) { name = "PercentageToken"; },
            [&](const DimensionToken&) { name = "DimensionToken"; },
            [&](const WhitespaceToken&) { name = "WhitespaceToken"; },
            [&](const CDOToken&) { name = "CDOToken"; },
            [&](const CDCToken&) { name = "CDCToken"; },
            [&](const ColonToken&) { name = "ColonToken"; },
            [&](const SemicolonToken&) { name = "SemicolonToken"; },
            [&](const CommaToken&) { name = "CommaToken"; },
            [&](const LeftSquareBracketToken&) { name = "LeftSquareBracketToken"; },
            [&](const RightSquareBracketToken&) { name = "RightSquareBracketToken"; },
            [&](const LeftParenthesesToken&) { name = "LeftParenthesesToken"; },
            [&](const RightParenthesesToken&) { name = "RightParenthesesToken"; },
            [&](const LeftCurlyBracketToken&) { name = "LeftCurlyBracketToken"; },
            [&](const RightCurlyBracketToken&) { name = "RightCurlyBracketToken"; },
            [&](const EOFToken&) { name = "EOFToken"; }
        }, token);
        
        return name;
    }

    class Tokenizer
    {
    public:
        Tokenizer();

        [[nodiscard]]
        auto run(std::string_view input) -> std::vector<Token>;

    private:
        // https://www.w3.org/TR/css-syntax-3/#consume-a-token
        auto consume_token() -> Token;

        // https://www.w3.org/TR/css-syntax-3/#consume-comments
        void consume_comments();

        // https://www.w3.org/TR/css-syntax-3/#whitespace
        auto is_whitespace(char c) const -> bool;

        // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
        auto is_ident_start(char c) const -> bool;

        auto next_chars_equals(std::string_view input) const -> bool;

    private:
        std::string m_input_stream;
        size_t m_current_char_idx = 0;
    };

}
