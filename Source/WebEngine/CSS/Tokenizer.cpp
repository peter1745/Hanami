#include "Tokenizer.hpp"

#include "Kori/Core.hpp"

#include <print>
#include <charconv>
#include <cmath>
#include <ranges>

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

    auto Tokenizer::tokenize(std::string_view input) -> std::vector<Token>
    {
        m_input_stream = input;

        Token current_token{};

        std::vector<Token> tokens;

        do
        {
            current_token = consume_token();
            tokens.emplace_back(current_token);
        } while (!std::holds_alternative<EOFToken>(current_token));

        return tokens;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-token
    auto Tokenizer::consume_token() -> Token
    {
        // Consume comments.

        // Consume the next input code point.
        const char c = consume_next_character();

        // EOF
        if (c == '\0' || m_reached_eof)
        {
            // Return an <EOF-token>.
            return EOFToken{};
        }

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
                    hash_token.type = HashToken::Type::ID;
                }

                // Consume an ident sequence, and set the <hash-token>’s value to the returned string.
                hash_token.value = consume_ident_sequence();

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
            return DelimToken{ c };
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
            --m_next_char_idx;
            return unwrap_numeric_token(consume_numeric_token());
        }

        // ident-start code point
        if (is_ident_start(c))
        {
            // Reconsume the current input code point
            --m_next_char_idx;

            // consume an ident-like token, and return it
            return consume_ident_like();
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

    // https://www.w3.org/TR/css-syntax-3/#consume-a-numeric-token
    auto Tokenizer::consume_numeric_token() -> NumericToken
    {
        // Consume a number and let number be the result.
        auto[type, number] = consume_number();

        // If the next 3 input code points would start an ident sequence, then:
        if (would_start_ident_sequence(m_input_stream[m_next_char_idx], m_input_stream[m_next_char_idx + 1], m_input_stream[m_next_char_idx + 2]))
        {
            // Create a <dimension-token> with the same value and type flag as number, and a unit set initially to the empty string.
            // Consume an ident sequence. Set the <dimension-token>’s unit to the returned value.
            // Return the <dimension-token>.
            return DimensionToken { type, number, consume_ident_sequence() };
        }

        // Otherwise, if the next input code point is U+0025 PERCENTAGE SIGN (%), consume it. Create a <percentage-token> with the same value as number, and return it.
        if (m_input_stream[m_next_char_idx] == '%')
        {
            consume_next_character();
            return PercentageToken{ number };
        }

        // Otherwise, create a <number-token> with the same value and type flag as number, and return it.
        return NumberToken{ type, number };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-number
    auto Tokenizer::consume_number() -> std::pair<NumericType, double>
    {
        // Initially set type to "integer". Let repr be the empty string.
        auto type = NumericType::Integer;
        auto repr = std::string{};

        // If the next input code point is U+002B PLUS SIGN (+) or U+002D HYPHEN-MINUS (-), consume it and append it to repr.
        if (m_input_stream[m_next_char_idx] == '+' || m_input_stream[m_next_char_idx] == '-')
        {
            repr += consume_next_character();
        }

        // While the next input code point is a digit, consume it and append it to repr.
        while (is_ascii_digit(m_input_stream[m_next_char_idx]))
        {
            repr += consume_next_character();
        }

        // If the next 2 input code points are U+002E FULL STOP (.) followed by a digit, then:
        if (m_input_stream[m_next_char_idx] == '.' && is_ascii_digit(m_input_stream[m_next_char_idx + 1]))
        {
            // Consume them.
            auto full_stop = consume_next_character();
            auto digit = consume_next_character();

            // Append them to repr.
            repr += full_stop;
            repr += digit;

            // Set type to "number".
            type = NumericType::Number;

            // While the next input code point is a digit, consume it and append it to repr.
            while (is_ascii_digit(m_input_stream[m_next_char_idx]))
            {
                repr += consume_next_character();
            }
        }

        // If the next 2 or 3 input code points are U+0045 LATIN CAPITAL LETTER E (E) or U+0065 LATIN SMALL LETTER E (e), optionally followed by U+002D HYPHEN-MINUS (-) or U+002B PLUS SIGN (+), followed by a digit, then:
        auto has_exponent_marker = std::tolower(m_input_stream[m_next_char_idx]) == 'e';
        uint8_t exponent_offset = (m_input_stream[m_next_char_idx + 1] == '+' || m_input_stream[m_next_char_idx + 1] == '-') ? 2 : 1;

        if (has_exponent_marker && is_ascii_digit(m_input_stream[m_next_char_idx + exponent_offset]))
        {
            // Consume them.
            // Append them to repr.
            for (uint8_t i = 0; i < exponent_offset; ++i)
            {
                repr += consume_next_character();
            }

            // Set type to "number".
            type = NumericType::Number;

            // While the next input code point is a digit, consume it and append it to repr.
            while (is_ascii_digit(m_input_stream[m_next_char_idx]))
            {
                repr += consume_next_character();
            }
        }

        // Convert repr to a number, and set the value to the returned value.
        // Return value and type.
        return { type, convert_string_to_number(repr) };
    }

    // https://www.w3.org/TR/css-syntax-3/#convert-a-string-to-a-number
    auto Tokenizer::convert_string_to_number(std::string_view repr) -> double
    {
        // Divide the string into seven components, in order from left to right:
        // A sign: a single U+002B PLUS SIGN (+) or U+002D HYPHEN-MINUS (-), or the empty string.
        auto sign = std::string_view{};

        if (repr.starts_with('+') || repr.starts_with('-'))
        {
            sign = repr.substr(0, 1);
        }

        // Let s be the number -1 if the sign is U+002D HYPHEN-MINUS (-); otherwise, let s be the number 1.
        int32_t s = sign.contains('-') ? -1 : 1;

        // An integer part: zero or more digits.
        size_t integer_end;
        for (integer_end = sign.length(); integer_end < repr.length(); ++integer_end)
        {
            if (!is_ascii_digit(repr[integer_end]))
            {
                break;
            }
        }

        auto integer_start = std::next(std::begin(repr), sign.length());
        auto integer_part = std::string_view{ integer_start, std::next(integer_start, integer_end) };

        // If there is at least one digit, let i be the number formed by interpreting the digits as a base-10 integer; otherwise, let i be the number 0.
        int32_t i = 0;
        std::from_chars(integer_part.begin(), integer_part.end(), i);

        // NOTE(Peter): String is unused
        // A decimal point: a single U+002E FULL STOP (.), or the empty string.
        auto decimal_start = std::ranges::find(repr, '.');

        // A fractional part: zero or more digits.
        auto fractional_range = std::ranges::find_last_if(decimal_start, std::end(repr), is_ascii_digit);
        auto fractional_part = std::string_view{ fractional_range.begin(), fractional_range.end() };

        // If there is at least one digit, let f be the number formed by interpreting the digits as a base-10 integer and d be the number of digits;
        // otherwise, let f and d be the number 0.
        int32_t f = 0;
        int32_t d = fractional_part.length();
        std::from_chars(fractional_part.begin(), fractional_part.end(), f);

        // An exponent indicator: a single U+0045 LATIN CAPITAL LETTER E (E) or U+0065 LATIN SMALL LETTER E (e), or the empty string.
        auto exponent_indicator_start = std::ranges::find_if(repr, [](char c){ return std::tolower(c) == 'e'; });

        // An exponent sign: a single U+002B PLUS SIGN (+) or U+002D HYPHEN-MINUS (-), or the empty string.
        auto exponent_sign_start = std::ranges::find_if(exponent_indicator_start, repr.end(), [](char c) { return c == '+' || c == '-'; });
        auto exponent_sign = std::string_view{ exponent_sign_start, 1 };

        // Let t be the number -1 if the sign is U+002D HYPHEN-MINUS (-); otherwise, let t be the number 1.
        int32_t t = exponent_sign.contains('-') ? -1 : 1;

        // An exponent: zero or more digits.
        auto exponent_range = std::ranges::find_last_if(exponent_sign_start, repr.end(), is_ascii_digit);
        auto exponent = std::string_view{ exponent_range.begin(), exponent_range.end() };

        // If there is at least one digit, let e be the number formed by interpreting the digits as a base-10 integer; otherwise, let e be the number 0.
        int32_t e = 0;
        std::from_chars(exponent.begin(), exponent.end(), e);

        // Return the number s·(i + f·10^-d)·10^te.
        return s * (i + f * std::pow(10.0, -d)) * std::pow(10.0, t * e);
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
