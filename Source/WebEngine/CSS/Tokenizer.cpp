#include "Tokenizer.hpp"

#include <print>

namespace Hanami::CSS {

    Tokenizer::Tokenizer()
    {

    }

    auto Tokenizer::run(std::string_view input) -> std::vector<Token>
    {
        m_input_stream = input;

        auto tokens = std::vector<Token>{};

        while (m_next_char_idx <= m_input_stream.size())
        {
            tokens.emplace_back(consume_token());

            if (std::holds_alternative<EOFToken>(tokens.back()))
            {
                break;
            }
        }

        return tokens;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-token
    auto Tokenizer::consume_token() -> Token
    {
        if (m_next_char_idx >= m_input_stream.length())
        {
            return EOFToken{};
        }

        consume_comments();

        auto codepoint = m_input_stream[m_next_char_idx++];

        if (is_whitespace(codepoint))
        {
            // Consume as much whitespace as possible
            while (m_next_char_idx < m_input_stream.length() && is_whitespace(m_input_stream[m_next_char_idx]))
            {
                ++m_next_char_idx;
            }

            return WhitespaceToken{};
        }

        if (codepoint == '"')
        {
            // Consume a string token and return it
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == '#')
        {
            // If the next input code point is an ident code point or the next two input code points are a valid escape, then:
            if (is_ident_code_point(m_input_stream[m_next_char_idx]) || two_are_valid_escape(m_input_stream[m_next_char_idx], m_input_stream[m_next_char_idx + 1]))
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
            return DelimToken{ codepoint };
        }

        if (codepoint == '\'')
        {
            // Consume a string token and return it
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == '(')
        {
            return LeftParenthesesToken{};
        }

        if (codepoint == ')')
        {
            return RightParenthesesToken{};
        }

        if (codepoint == '+')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == ',')
        {
            return CommaToken{};
        }

        if (codepoint == '-')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, if the next 2 input code points are U+002D HYPHEN-MINUS U+003E GREATER-THAN SIGN (->), consume them and return a <CDC-token>.

            // Otherwise, if the input stream starts with an ident sequence, reconsume the current input code point, consume an ident-like token, and return it.

            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == '.')
        {
            // If the input stream starts with a number, reconsume the current input code point, consume a numeric token, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == ':')
        {
            return ColonToken{};
        }

        if (codepoint == ';')
        {
            return SemicolonToken{};
        }

        if (codepoint == '<')
        {
            // If the next 3 input code points are U+0021 EXCLAMATION MARK U+002D HYPHEN-MINUS U+002D HYPHEN-MINUS (!--), consume them and return a <CDO-token>.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == '@')
        {
            // If the next 3 input code points would start an ident sequence, consume an ident sequence, create an <at-keyword-token> with its value set to the returned value, and return it.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == '[')
        {
            return LeftSquareBracketToken{};
        }

        if (codepoint == '\\')
        {
            // If the input stream starts with a valid escape, reconsume the current input code point, consume an ident-like token, and return it.
            // Otherwise, this is a parse error. Return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (codepoint == ']')
        {
            return RightSquareBracketToken{};
        }

        if (codepoint == '{')
        {
            return LeftCurlyBracketToken{};
        }

        if (codepoint == '}')
        {
            return RightCurlyBracketToken{};
        }

        if (is_ascii_digit(codepoint))
        {
            // Reconsume the current input code point, consume a numeric token, and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        if (is_ident_start(codepoint))
        {
            // Reconsume the current input code point, consume an ident-like token, and return it.
            reconsume_current();
            return consume_ident_like();
        }

        if (codepoint == '\0')
        {
            return EOFToken{};
        }

        return DelimToken { codepoint };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-comments
    void Tokenizer::consume_comments()
    {
        while (next_chars_equals("/*"))
        {
            m_next_char_idx = m_input_stream.find("*/", m_next_char_idx) + 2;
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-an-ident-like-token
    auto Tokenizer::consume_ident_like() -> Token
    {
        auto string = consume_ident_sequence();

        // If string’s value is an ASCII case-insensitive match for "url", and the next input code point is U+0028 LEFT PARENTHESIS ((), consume it.
        if (equals_case_insensitive(string, "url") && m_input_stream[m_next_char_idx] == '(')
        {
            ++m_next_char_idx;

            // While the next two input code points are whitespace, consume the next input code point.
            while (is_whitespace(m_input_stream[m_next_char_idx]) && is_whitespace(m_input_stream[m_next_char_idx + 1]))
            {
                ++m_next_char_idx;
            }

            // If the next one or two input code points are U+0022 QUOTATION MARK ("), U+0027 APOSTROPHE ('),
            // or whitespace followed by U+0022 QUOTATION MARK (") or U+0027 APOSTROPHE ('),
            // then create a <function-token> with its value set to string and return it.
            if (
                m_input_stream[m_next_char_idx] == '"' || m_input_stream[m_next_char_idx] == '\'' ||
                (is_whitespace(m_input_stream[m_next_char_idx]) && (m_input_stream[m_next_char_idx + 1] == '"' || m_input_stream[m_next_char_idx + 1] == '\''))
            )
            {
                return FunctionToken{ string };
            }

            // Otherwise, consume a url token, and return it.
            HANAMI_NOT_IMPLEMENTED("Consume a URL token and return it");
        }

        // Otherwise, if the next input code point is U+0028 LEFT PARENTHESIS ((), consume it.
        // Create a <function-token> with its value set to string and return it.
        if (m_input_stream[m_next_char_idx] == '(')
        {
            ++m_next_char_idx;
            return FunctionToken{ string };
        }

        // Otherwise, create an <ident-token> with its value set to string and return it.
        return IdentToken{ string };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-an-ident-sequence
    auto Tokenizer::consume_ident_sequence() -> std::string
    {
        auto result = std::string{};

        while (true)
        {
            auto token = m_input_stream[m_next_char_idx++];

            if (is_ident_code_point(token))
            {
                result += token;
                continue;
            }

            if (two_are_valid_escape(token, m_input_stream[m_next_char_idx]))
            {
                HANAMI_NOT_IMPLEMENTED("Consume an escaped code point. Append the returned code point to result.");
            }

            reconsume_current();
            return result;
        }

        return "";
    }

    // https://www.w3.org/TR/css-syntax-3/#whitespace
    auto Tokenizer::is_whitespace(char c) const -> bool
    {
        return c == '\n' || c == '\t' || c == ' ';
    }

    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    auto Tokenizer::is_ident_start(char c) const -> bool
    {
        // TODO(Peter): non-ASCII code point
        return is_ascii_alpha(c) || c == '_';
    }

    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    auto Tokenizer::is_ident_code_point(char c) const -> bool
    {
        return is_ident_start(c) || is_ascii_digit(c) || c == '-';
    }

    // https://www.w3.org/TR/css-syntax-3/#check-if-two-code-points-are-a-valid-escape
    auto Tokenizer::two_are_valid_escape(char c0, char c1) const -> bool
    {
        return c0 == '\\' && c1 !='\n';
    }
    
    // https://www.w3.org/TR/css-syntax-3/#check-if-three-code-points-would-start-an-ident-sequence
    auto Tokenizer::would_start_ident_sequence(char c0, char c1, char c2) const -> bool
    {
        if (c0 == '-')
        {
            // If the second code point is an ident-start code point or a U+002D HYPHEN-MINUS, 
            // or the second and third code points are a valid escape, return true. Otherwise, return false.
            return is_ident_start(c1) || c1 == '-' || two_are_valid_escape(c1, c2);
        }
        
        if (c0 == '\\')
        {
            // If the first and second code points are a valid escape, return true. Otherwise, return false.
            return two_are_valid_escape(c0, c1);
        }
        
        return is_ident_start(c0);
    }

    auto Tokenizer::next_chars_equals(std::string_view input) const -> bool
    {
        if (m_next_char_idx + input.length() >= m_input_stream.length())
        {
            return false;
        }

        return std::string_view{ &m_input_stream[m_next_char_idx], input.length() } == input;
    }

    // https://www.w3.org/TR/css-syntax-3/#reconsume-the-current-input-code-point
    void Tokenizer::reconsume_current()
    {
        --m_next_char_idx;
    }

}
