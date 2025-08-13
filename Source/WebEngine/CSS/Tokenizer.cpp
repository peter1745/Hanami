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

        while (m_current_char_idx <= m_input_stream.size())
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
        if (m_current_char_idx >= m_input_stream.length())
        {
            return EOFToken{};
        }
        
        consume_comments();

        auto codepoint = m_input_stream[m_current_char_idx++];

        if (is_whitespace(codepoint))
        {
            // Consume as much whitespace as possible
            while (m_current_char_idx < m_input_stream.length() && is_whitespace(m_input_stream[m_current_char_idx]))
            {
                ++m_current_char_idx;
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
                // Create a <hash-token>.
                // If the next 3 input code points would start an ident sequence, set the <hash-token>’s type flag to "id".
                // Consume an ident sequence, and set the <hash-token>’s value to the returned string.
                // Return the <hash-token>.
            // Otherwise, return a <delim-token> with its value set to the current input code point.
            HANAMI_NOT_IMPLEMENTED();
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
            HANAMI_NOT_IMPLEMENTED();
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
            m_current_char_idx = m_input_stream.find("*/", m_current_char_idx) + 2;
        }
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

    auto Tokenizer::next_chars_equals(std::string_view input) const -> bool
    {
        if (m_current_char_idx + input.length() >= m_input_stream.length())
        {
            return false;
        }

        return std::string_view{ &m_input_stream[m_current_char_idx], input.length() } == input;
    }
}
