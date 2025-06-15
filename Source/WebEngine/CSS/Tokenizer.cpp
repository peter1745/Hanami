#include "Tokenizer.hpp"

#include <print>

#include "Kori/Core.hpp"

namespace Hanami::CSS {

    // https://www.w3.org/TR/css-syntax-3/#whitespace
    static auto is_css_whitespace(char c) -> bool
    {
        return c == '\n' || c == '\t' || c == ' ';
    }

    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    static auto is_ident_start(char c) -> bool
    {
        return is_ascii_alpha(c) /* || is_non_ascii_code_point(c) */ || c == '_';
    }

    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    static auto is_ident_code_point(char c) -> bool
    {
        return is_ident_start(c) || is_ascii_digit(c) || c == '-';
    }

    static auto is_valid_escape(char c0, char c1) -> bool
    {
        return c0 == '\\' && c1 != '\n';
    }

    static auto would_start_ident_sequence(char c0, char c1, char c2) -> bool
    {
        // Look at the first code point:
        // U+002D HYPHEN-MINUS
        if (c0 == '-')
        {
            // If the second code point is an ident-start code point or a U+002D HYPHEN-MINUS, or the second and third code points are a valid escape, return true.
            // Otherwise, return false.
            return is_ident_start(c1) || c1 == '-' || is_valid_escape(c1, c2);
        }

        // ident-start code point
        if (is_ident_start(c0))
        {
            // Return true.
            return true;
        }

        // U+005C REVERSE SOLIDUS (\)
        if (c0 == '\\')
        {
            // If the first and second code points are a valid escape, return true. Otherwise, return false.
            return is_valid_escape(c0, c1);
        }

        // anything else
        // Return false.
        return false;
    }

    void Tokenizer::tokenize(std::string_view input)
    {
        m_input_stream = input;

        Token current_token{};

        std::vector<Token> tokens;

        do
        {
            current_token = consume_token();
            tokens.emplace_back(current_token);
        } while (!std::holds_alternative<EOFToken>(current_token));

        for (const auto& token : tokens)
        {
            std::visit(Kori::VariantOverloadSet {
                [](IdentToken token) { std::println("IdentToken({})", token.value); },
                [](FunctionToken) { std::println("FunctionToken"); },
                [](AtKeywordToken) { std::println("AtKeywordToken"); },
                [](HashToken token) { std::println("HashToken({})", token.value); },
                [](StringToken) { std::println("StringToken"); },
                [](BadStringToken) { std::println("BadStringToken"); },
                [](URLToken) { std::println("URLToken"); },
                [](BadURLToken) { std::println("BadURLToken"); },
                [](DelimToken token) { std::println("DelimToken({})", token.value); },
                [](NumberToken) { std::println("NumberToken"); },
                [](PercentageToken) { std::println("PercentageToken"); },
                [](DimensionToken) { std::println("DimensionToken"); },
                [](WhitespaceToken) { std::println("WhitespaceToken"); },
                [](CDOToken) { std::println("CDOToken"); },
                [](CDCToken) { std::println("CDCToken"); },
                [](ColonToken) { std::println("ColonToken"); },
                [](SemicolonToken) { std::println("SemicolonToken"); },
                [](CommaToken) { std::println("CommaToken"); },
                [](OpenSquareBracketToken) { std::println("OpenSquareBracketToken"); },
                [](CloseSquareBracketToken) { std::println("CloseSquareBracketToken"); },
                [](OpenParenthesesToken) { std::println("OpenParenthesesToken"); },
                [](CloseParenthesesToken) { std::println("CloseParenthesesToken"); },
                [](OpenCurlyBracketToken) { std::println("OpenCurlyBracketToken"); },
                [](CloseCurlyBracketToken) { std::println("CloseCurlyBracketToken"); },
                [](EOFToken) { std::println("EOFToken"); },
            }, token);
        }
    }

    auto Tokenizer::consume_token() -> Token
    {
        // Consume comments.

        // Consume the next input code point.
        const char c = consume_next_character();

        // whitespace
        if (is_css_whitespace(c))
        {
            // Consume as much whitespace as possible.
            while (is_css_whitespace(consume_next_character())) {}

            // Backtrack one because the above loop will overconsume by 1
            --m_next_char_idx;

            // Return a <whitespace-token>.
            return WhitespaceToken{};
        }

        // U+0022 QUOTATION MARK (")
        if (c == '"')
        {
            // Consume a string token and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+0023 NUMBER SIGN (#)
        if (c == '#')
        {
            // If the next input code point is an ident code point or the next two input code points are a valid escape, then:
            if (is_ident_code_point(m_input_stream[m_next_char_idx]) || is_valid_escape(m_input_stream[m_next_char_idx], m_input_stream[m_next_char_idx + 1]))
            {
                // Create a <hash-token>.
                auto hash_token = HashToken{};

                // If the next 3 input code points would start an ident sequence, set the <hash-token>’s type flag to "id".
                if (would_start_ident_sequence(m_input_stream[m_next_char_idx], m_input_stream[m_next_char_idx + 1], m_input_stream[m_next_char_idx + 2]))
                {
                    // Consume an ident sequence, and set the <hash-token>’s value to the returned string.
                    hash_token.value = consume_ident_sequence();
                }

                // Return the <hash-token>.
                return hash_token;
            }

            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+0027 APOSTROPHE (')
        if (c == '\'')
        {
            // Consume a string token and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+0028 LEFT PARENTHESIS (()
        if (c == '(')
        {
            // Return a <(-token>.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+0029 RIGHT PARENTHESIS ())
        if (c == '(')
        {
            // Return a <)-token>.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+002B PLUS SIGN (+)
        if (c == '+')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+002C COMMA (,)
        if (c == ',')
        {
            // Return a <comma-token>.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+002D HYPHEN-MINUS (-)
        if (c == '-')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, if the next 2 input code points are U+002D HYPHEN-MINUS U+003E GREATER-THAN SIGN (->), consume them and return a <CDC-token>.
            // Otherwise, if the input stream starts with an ident sequence, reconsume the current input code point, consume an ident-like token, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+002E FULL STOP (.)
        if (c == '.')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+003A COLON (:)
        if (c == ':')
        {
            // Return a <colon-token>.
            return ColonToken{};
        }

        // U+003B SEMICOLON (;)
        if (c == ';')
        {
            // Return a <semicolon-token>.
            return SemicolonToken{};
        }

        // U+003C LESS-THAN SIGN (<)
        if (c == '<')
        {
            // If the next 3 input code points are U+0021 EXCLAMATION MARK U+002D HYPHEN-MINUS U+002D HYPHEN-MINUS (!--), consume them and return a <CDO-token>.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+0040 COMMERCIAL AT (@)
        if (c == '@')
        {
            // If the next 3 input code points would start an ident sequence, consume an ident sequence, create an <at-keyword-token> with its value set to the returned value, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+005B LEFT SQUARE BRACKET ([)
        if (c == '[')
        {
            // Return a <[-token>.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+005C REVERSE SOLIDUS (\)
        if (c == '\\')
        {
            // If the input stream starts with a valid escape, reconsume the current input code point, consume an ident-like token, and return it.
            // Otherwise, this is a parse error. Return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+005D RIGHT SQUARE BRACKET (])
        if (c == ']')
        {
            // Return a <]-token>.
            HANAMI_NOT_IMPLEMENTED();
        }

        // U+007B LEFT CURLY BRACKET ({)
        if (c == '{')
        {
            // Return a <{-token>.
            return OpenCurlyBracketToken{};
        }

        // U+007D RIGHT CURLY BRACKET (})
        if (c == '}')
        {
            // Return a <}-token>.
            return CloseCurlyBracketToken{};
        }

        // digit
        if (is_ascii_digit(c))
        {
            // Reconsume the current input code point, consume a numeric token, and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // ident-start code point
        if (is_ident_start(c))
        {
            // Reconsume the current input code point
            --m_next_char_idx;

            // consume an ident-like token, and return it
            return consume_ident_like();
        }

        // EOF
        if (c == '\0' || m_reached_eof)
        {
            // Return an <EOF-token>.
            return EOFToken{};
        }

        // anything else
        // Return a <delim-token> with its value set to the current input code point.
        return DelimToken{ c };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-an-ident-like-token
    auto Tokenizer::consume_ident_like() -> Token
    {
        // Consume an ident sequence, and let string be the result.
        auto string = consume_ident_sequence();

        // If string’s value is an ASCII case-insensitive match for "url", and the next input code point is U+0028 LEFT PARENTHESIS (()
        if (equals_case_insensitive(string, "url") && m_input_stream[m_next_char_idx] == '(')
        {
            // consume it.
            (void)consume_next_character();

            // While the next two input code points are whitespace
            while (is_css_whitespace(m_input_stream[m_next_char_idx]) && is_css_whitespace(m_input_stream[m_next_char_idx + 1]))
            {
                // consume the next input code point.
                (void)consume_next_character();
            }

            // If the next one or two input code points are U+0022 QUOTATION MARK ("), U+0027 APOSTROPHE ('),
            // or whitespace followed by U+0022 QUOTATION MARK (") or U+0027 APOSTROPHE ('),
            if (
                (m_input_stream[m_next_char_idx] == '"' && m_input_stream[m_next_char_idx + 1] == '\'') ||
                (is_css_whitespace(m_input_stream[m_next_char_idx]) && (m_input_stream[m_next_char_idx + 1] == '"' || m_input_stream[m_next_char_idx + 1] == '\''))
            )
            {
                // then create a <function-token> with its value set to string and return it.
                return FunctionToken{ string };
            }

            // Otherwise, consume a url token, and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // Otherwise, if the next input code point is U+0028 LEFT PARENTHESIS (()
        if (m_input_stream[m_next_char_idx] == '(')
        {
            // consume it.
            (void)consume_next_character();

            // Create a <function-token> with its value set to string and return it.
            return FunctionToken{ string };
        }

        // Otherwise, create an <ident-token> with its value set to string and return it.
        return IdentToken{ string };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-an-ident-sequence
    auto Tokenizer::consume_ident_sequence() -> std::string
    {
        // Let result initially be an empty string.
        auto result = std::string{};

        while (true)
        {
            // Repeatedly consume the next input code point from the stream:
            const char c = consume_next_character();

            // ident code point
            if (is_ident_code_point(c))
            {
                // Append the code point to result.
                result += c;
                continue;
            }

            // the stream starts with a valid escape
            if (is_valid_escape(c, m_input_stream[m_next_char_idx]))
            {
                // Consume an escaped code point. Append the returned code point to result.
                HANAMI_NOT_IMPLEMENTED("Unsure what \"the stream *starts* with a valid escape means\"");
            }

            // anything else
            // Reconsume the current input code point.
            --m_next_char_idx;

            // Return result.
            return result;
        }
    }

    auto Tokenizer::consume_next_character() noexcept -> char
    {
        if (m_next_char_idx + 1 > m_input_stream.length())
        {
            // EOF, unable to consume more characters.
            m_reached_eof = true;
            return '\0';
        }

        return m_input_stream[m_next_char_idx++];
    }

}
