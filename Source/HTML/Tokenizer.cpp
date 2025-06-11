#include "Tokenizer.hpp"
#include "Core/Core.hpp"

#include <kori/core.hpp>

#include <regex>
#include <print>
#include <algorithm>
#include <cstring>
#include <signal.h>

namespace Hanami::HTML {

    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    static auto is_ascii_upper_alpha(char c) -> bool
    {
        return c >= 'A' && c <= 'Z';
    }

    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    static auto is_ascii_lower_alpha(char c) -> bool
    {
        return c >= 'a' && c <= 'z';
    }

    // https://infra.spec.whatwg.org/#ascii-alpha
    static auto is_ascii_alpha(char c) -> bool
    {
        return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
    }

    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    static auto is_ascii_alpha_numeric(char c) -> bool
    {
        return (c >= '0' && c <= '9') || is_ascii_alpha(c);
    }

    void Tokenizer::print_token(const Token& t)
    {
        std::visit(kori::VariantOverloadSet {
            [](const DOCTYPEToken& token)
            {
                std::println("DOCTYPE(name = {}, force_quirks = {})", token.name, token.force_quirks);
            },
            [](const StartTagToken& token)
            {
                std::println("StartTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const EndTagToken& token)
            {
                std::println("EndTagToken(name = {}, self_closing = {})", token.name, token.self_closing);
                for (const auto& attr : token.attributes)
                {
                    std::println("\tAttribute(name = {}, value = {})", attr.name, attr.value);
                }
            },
            [](const CommentToken& token)
            {
                std::println("CommentToken(data = {})", token.data);
            },
            [](const CharacterToken& token)
            {
                if (token.data == '\n' || token.data == ' ')
                {
                    return;
                }

                std::println("CharacterToken(data = {})", token.data);
            },
            [](const EOFToken&)
            {
                std::println("EOFToken");
            }
        }, t);
    }

    void Tokenizer::start(std::string_view input, EmitTokenFunc func)
    {
        m_emit_token = std::move(func);
        m_input_stream = input;
        m_state = State::Data;

        while (true)
        {
            // TODO(Peter): Check parser pause flag and abort if set

            if (process_next_token() == ProcessResult::Abort)
            {
                break;
            }
        }
    }

    void Tokenizer::emit_token(const Token& token)
    {
        if (!m_emit_token)
        {
            raise(SIGTRAP);
            return;
        }

        std::println("Tokenizer: Emitting token:");
        print_token(token);

        if (const auto* start_tag = std::get_if<StartTagToken>(&token); start_tag)
        {
            m_last_emitted_start_token_name = start_tag->name;
        }

        m_emit_token(token);
    }

    auto Tokenizer::consume_multiple_chars(size_t count) noexcept -> std::string_view
    {
        // Try to consume count number of characters
        const auto view = m_input_stream.substr(m_current_char_idx, count);

        // Advance by the amount of characters we successfully consumed (might be smaller than count)
        m_current_char_idx += view.length();

        // Return
        return view;
    }

    auto Tokenizer::consume_next_character() noexcept -> char
    {
        if (m_current_char_idx + 1 > m_input_stream.length())
        {
            // EOF, unable to consume more characters.
            return '\0';
        }

        return m_input_stream[m_current_char_idx++];
    }

    void Tokenizer::reconsume_in(State state) noexcept
    {
        --m_current_char_idx;
        m_state = state;
    }

    auto Tokenizer::reached_eof() const noexcept -> bool
    {
        return m_current_char_idx >= m_input_stream.length();
    }

    auto Tokenizer::next_characters_equals(std::string_view chars, bool case_insensitive) const noexcept -> bool
    {
        if (m_current_char_idx + chars.length() >= m_input_stream.length())
        {
            return false;
        }

        if (!case_insensitive)
        {
            return m_input_stream.compare(m_current_char_idx, chars.length(), chars) == 0;
        }

        return equals_case_insensitive(m_input_stream.substr(m_current_char_idx, chars.length()) , chars);
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#appropriate-end-tag-token
    auto Tokenizer::current_is_appropriate_end_tag() const noexcept -> bool
    {
        const auto* end_tag = std::get_if<EndTagToken>(&m_current_token);

        if (!end_tag || m_last_emitted_start_token_name.empty())
        {
            return false;
        }

        return end_tag->name == m_last_emitted_start_token_name;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#tokenization
    auto Tokenizer::process_next_token() -> ProcessResult
    {
        switch (m_state)
        {
            case State::Data:
            {
                // Consume the next input character
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                if (c == '&') // U+0026 AMPERSAND (&)
                {
                    // Set the return state to the data state.
                    m_return_state = State::Data;

                    // Switch to the character reference state.
                    m_state = State::CharacterReference;
                    break;
                }

                if (c == '<') // U+003C LESS-THAN SIGN (<)
                {
                    // Switch to the tag open state.
                    m_state = State::TagOpen;
                    break;
                }

                if (c == '\0') // U+0000 NULL
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Emit the current input character as a character token.
                    emit_token(CharacterToken{ c });
                    break;
                }

                // Anything else
                // Emit the current input character as a character token.
                emit_token(CharacterToken{ c });
                break;
            }
            case State::TagOpen:
            {
                // Consume the next input character
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-before-tag-name parse error.
                    // parse_error(ErrorType::EOFBeforeTagName);

                    // Emit a U+003C LESS-THAN SIGN character token and an end-of-file token.
                    emit_token(CharacterToken{ '<' });
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                if (c == '!') // U+0021 EXCLAMATION MARK (!)
                {
                    // Switch to the markup declaration open state.
                    m_state = State::MarkupDeclarationOpen;
                    break;
                }

                if (c == '/') // U+002F SOLIDUS (/)
                {
                    // Switch to the end tag open state.
                    m_state = State::EndTagOpen;
                    break;
                }

                if (is_ascii_alpha(c)) // ASCII alpha
                {
                    // Create a new start tag token, set its tag name to the empty string.
                    m_current_token = StartTagToken{ "" };

                    // Reconsume in the tag name state.
                    reconsume_in(State::TagName);
                    break;
                }

                if (c == '?') // U+003F QUESTION MARK (?)
                {
                    // This is an unexpected-question-mark-instead-of-tag-name parse error.
                    // parse_error(ErrorType::UnexpectedQuestionMarkInsteadOfTagName);

                    // Create a comment token whose data is the empty string.
                    m_current_token = CommentToken{ "" };

                    // Reconsume in the bogus comment state.
                    reconsume_in(State::BogusComment);
                    break;
                }

                // Anything else
                // This is an invalid-first-character-of-tag-name parse error.
                // parse_error(ErrorType::InvalidFirstCharacterOfTagName);

                // Emit a U+003C LESS-THAN SIGN character token.
                emit_token(CharacterToken{ '<' });

                // Reconsume in the data state.
                reconsume_in(State::Data);
                break;
            }
            case State::EndTagOpen:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-before-tag-name parse error.
                    // parse_error(ErrorType::EOFBeforeTagName);

                    // Emit a U+003C LESS-THAN SIGN character token, a U+002F SOLIDUS character token and an end-of-file token.
                    emit_token(CharacterToken{ '<' });
                    emit_token(CharacterToken{ '/' });
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // ASCII alpha
                if (is_ascii_alpha(c))
                {
                    // Create a new end tag token, set its tag name to the empty string.
                    m_current_token = EndTagToken { "" };

                    // Reconsume in the tag name state.
                    reconsume_in(State::TagName);
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // This is a missing-end-tag-name parse error.
                    // parse_error(ErrorType::MissingEndTagName);

                    // Switch to the data state.
                    m_state = State::Data;
                    break;
                }

                // Anything else
                // This is an invalid-first-character-of-tag-name parse error.
                // parse_error(ErrorType::InvalidFirstCharacterOfTagName);

                // Create a comment token whose data is the empty string.
                m_current_token = CommentToken{ "" };

                // Reconsume in the bogus comment state.
                reconsume_in(State::BogusComment);
                break;
            }
            case State::MarkupDeclarationOpen:
            {
                // If the next few characters are:
                // Two U+002D HYPHEN-MINUS characters (-)
                if (next_characters_equals("--"))
                {
                    // Consume those two characters
                    consume_multiple_chars(2);

                    // Create a comment token whose data is the empty string
                    m_current_token = CommentToken{ "" };

                    // Switch to the comment start state.
                    m_state = State::CommentStart;
                    break;
                }

                // ASCII case-insensitive match for the word "DOCTYPE"
                if (next_characters_equals("DOCTYPE", true))
                {
                    // Consume those characters
                    consume_multiple_chars(std::strlen("DOCTYPE"));

                    // Switch to the DOCTYPE state.
                    m_state = State::DOCTYPE;
                    break;
                }

                // The string "[CDATA[" (the five uppercase letters "CDATA" with a U+005B LEFT SQUARE BRACKET character before and after)
                if (next_characters_equals("[CDATA["))
                {
                    // Consume those characters.
                    consume_multiple_chars(std::strlen("[CDATA["));
                    
                    // If there is an adjusted current node and it is not an element in the HTML namespace, then switch to the CDATA section state.
                    // Otherwise, this is a cdata-in-html-content parse error.
                    // Create a comment token whose data is the "[CDATA[" string.
                    // Switch to the bogus comment state.
                    raise(SIGTRAP);
                    break;
                }

                // Anything else
                // This is an incorrectly-opened-comment parse error.
                // parse_error(ErrorType::IncorrectlyOpenedComment);

                // Create a comment token whose data is the empty string.
                m_current_token = CommentToken{ "" };

                // Switch to the bogus comment state (don't consume anything in the current state).
                m_state = State::BogusComment;
                break;
            }
            case State::DOCTYPE:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-doctype parse error.
                    // parse_error(ErrorType::EOFInDOCTYPE);

                    // Create a new DOCTYPE token.
                    // Set its force-quirks flag to on.
                    m_current_token = DOCTYPEToken {
                        .force_quirks = true
                    };

                    // Emit the current token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Switch to the before DOCTYPE name state.
                    m_state = State::BeforeDOCTYPEName;
                    break;
                }

                if (c == '>') // U+003E GREATER-THAN SIGN (>)
                {
                    // Reconsume in the before DOCTYPE name state.
                    reconsume_in(State::BeforeDOCTYPEName);
                    break;
                }

                // Anything else
                // This is a missing-whitespace-before-doctype-name parse error.
                // parse_error(ErrorType::MissingWhitespaceBeforeDOCTYPEName);

                // Reconsume in the before DOCTYPE name state.
                reconsume_in(State::BeforeDOCTYPEName);
                break;
            }
            case State::BeforeDOCTYPEName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-doctype parse error.
                    // parse_error(ErrorType::EOFInDOCTYPE);

                    // Create a new DOCTYPE token.
                    // Set its force-quirks flag to on.
                    m_current_token = DOCTYPEToken {
                        .force_quirks = true
                    };

                    // Emit the current token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Ignore the character.
                    break;
                }

                // ASCII upper alpha
                if (is_ascii_upper_alpha(c))
                {
                    // Create a new DOCTYPE token.
                    // Set the token's name to the lowercase version of the current input character (add 0x0020 to the character's code point).
                    m_current_token = DOCTYPEToken {
                        .name = std::string{ static_cast<char>(std::tolower(c)) }
                    };

                    // Switch to the DOCTYPE name state.
                    m_state = State::DOCTYPEName;
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Create a new DOCTYPE token.
                    // Set the token's name to a U+FFFD REPLACEMENT CHARACTER character.
                    m_current_token = DOCTYPEToken {
                        .name = "�"
                    };

                    // Switch to the DOCTYPE name state.
                    m_state = State::DOCTYPEName;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // This is a missing-doctype-name parse error.
                    // parse_error(ErrorType::MissingDOCTYPEName);

                    // Create a new DOCTYPE token.
                    // Set its force-quirks flag to on.
                    m_current_token = DOCTYPEToken { .force_quirks = true };

                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current token.
                    emit_token(m_current_token);
                    break;
                }

                // Anything else
                // Create a new DOCTYPE token.
                // Set the token's name to the current input character.
                m_current_token = DOCTYPEToken { .name = { c } };

                // Switch to the DOCTYPE name state.
                m_state = State::DOCTYPEName;
                break;
            }
            case State::DOCTYPEName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-doctype parse error.
                    // parse_error(ErrorType::EOFInDOCTYPE);

                    // Set the current DOCTYPE token's force-quirks flag to on.
                    std::get<DOCTYPEToken>(m_current_token).force_quirks = true;

                    // Emit the current DOCTYPE token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Switch to the after DOCTYPE name state.
                    m_state = State::AfterDOCTYPEName;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current DOCTYPE token.
                    emit_token(m_current_token);
                    break;
                }

                // ASCII upper alpha
                if (is_ascii_upper_alpha(c))
                {
                    // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current DOCTYPE token's name.
                    std::get<DOCTYPEToken>(m_current_token).name += static_cast<char>(std::tolower(c));
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the current DOCTYPE token's name.
                    std::get<DOCTYPEToken>(m_current_token).name += "�";
                    break;
                }

                // Anything else
                // Append the current input character to the current DOCTYPE token's name.
                std::get<DOCTYPEToken>(m_current_token).name += c;
                break;
            }
            case State::CharacterReference:
            {
                // Set the temporary buffer to the empty string.
                m_temporary_buffer = "";

                // Append a U+0026 AMPERSAND (&) character to the temporary buffer.
                m_temporary_buffer.append("&");

                // Consume the next input character
                const char c = consume_next_character();

                if (is_ascii_alpha_numeric(c)) // ASCII alphanumeric
                {
                    // Reconsume in the named character reference state.
                    reconsume_in(State::NamedCharacterReference);
                    break;
                }

                if (c == '#') // U+0023 NUMBER SIGN (#)
                {
                    // Append the current input character to the temporary buffer.
                    m_temporary_buffer += c;

                    // Switch to the numeric character reference state.
                    m_state = State::NumericCharacterReference;
                    break;
                }

                // Anything else
                // Flush code points consumed as a character reference.
                for (auto character : m_temporary_buffer)
                {
                    emit_token(CharacterToken{ character });
                }

                // Reconsume in the return state.
                reconsume_in(m_return_state);
                break;
            }
            case State::NamedCharacterReference:
            {
                // TODO(Peter): Parse the JSON file mentioned here: https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references
                //              in the future to get a list of all valid named character references.
                raise(SIGTRAP);

                // Consume the maximum number of characters possible, where the consumed characters are one of the identifiers in the first column of the named character references table.
                while (true)
                {
                    const char c = consume_next_character();

                    if (!is_ascii_alpha(c))
                    {
                        // Matched as much as possible.
                        // NOTE(Peter): Should we backtrack one?
                        break;
                    }

                    // Append each character to the temporary buffer when it's consumed.
                    m_temporary_buffer += c;
                }

                m_temporary_buffer += ";";

                // If there is a match
                // If the character reference was consumed as part of an attribute, and the last character matched is not a U+003B SEMICOLON character (;), and the next input character is either a U+003D EQUALS SIGN character (=) or an ASCII alphanumeric, then, for historical reasons, flush code points consumed as a character reference and switch to the return state.
                // Otherwise:
                // If the last character matched is not a U+003B SEMICOLON character (;), then this is a missing-semicolon-after-character-reference parse error.
                // Set the temporary buffer to the empty string. Append one or two characters corresponding to the character reference name (as given by the second column of the named character references table) to the temporary buffer.
                // Flush code points consumed as a character reference. Switch to the return state.
                // Otherwise
                // Flush code points consumed as a character reference. Switch to the ambiguous ampersand state.
                break;
            }
            case State::TagName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-tag parse error.
                    // parse_error(ErrorType::EOFInTag);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Switch to the before attribute name state.
                    m_state = State::BeforeAttributeName;
                    break;
                }

                // U+002F SOLIDUS (/)
                if (c == '/')
                {
                    // Switch to the self-closing start tag state.
                    m_state = State::SelfClosingStartTag;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current tag token.
                    emit_token(m_current_token);
                    break;
                }

                // ASCII upper alpha
                if (is_ascii_upper_alpha(c))
                {
                    // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current tag token's tag name.
                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token)
                        {
                            token.name += static_cast<char>(std::tolower(c));
                        },
                        [&](EndTagToken& token)
                        {
                            token.name += static_cast<char>(std::tolower(c));
                        },
                        [](auto&&){ raise(SIGTRAP); }
                    }, m_current_token);
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the current tag token's tag name.
                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token)
                        {
                            token.name += "�";
                        },
                        [&](EndTagToken& token)
                        {
                            token.name += "�";
                        },
                        [](auto&&){ raise(SIGTRAP); }
                    }, m_current_token);
                    break;
                }

                // Anything else
                // Append the current input character to the current tag token's tag name.
                std::visit(kori::VariantOverloadSet {
                    [&](StartTagToken& token)
                    {
                        token.name += static_cast<char>(std::tolower(c));
                    },
                    [&](EndTagToken& token)
                    {
                        token.name += static_cast<char>(std::tolower(c));
                    },
                    [](auto&&){ raise(SIGTRAP); }
                }, m_current_token);
                break;
            }
            case State::SelfClosingStartTag:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-tag parse error.
                    // parse_error(ErrorType::EOFInTag);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Set the self-closing flag of the current tag token.
                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token)
                        {
                            token.self_closing = true;
                        },
                        [&](EndTagToken& token)
                        {
                            token.self_closing = true;
                        },
                        [](auto&&){ raise(SIGTRAP); }
                    }, m_current_token);

                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current tag token.
                    emit_token(m_current_token);
                    break;
                }

                // Anything else
                // This is an unexpected-solidus-in-tag parse error.
                // parse_error(ErrorType::UnexpectedSolidusInTag);

                // Reconsume in the before attribute name state.
                reconsume_in(State::BeforeAttributeName);
                break;
            }
            case State::BeforeAttributeName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // Reconsume in the after attribute name state.
                    reconsume_in(State::AfterAttributeName);
                    break;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Ignore the character.
                    break;
                }

                // U+002F SOLIDUS (/)
                // U+003E GREATER-THAN SIGN (>)
                // U+003D EQUALS SIGN (=)
                if (c == '/' || c == '>' || c == '=')
                {
                    // This is an unexpected-equals-sign-before-attribute-name parse error.
                    // parse_error(ErrorType::UnexpectedEqualsSignBeforeAttributeName);

                    // Start a new attribute in the current tag token.
                    // Set that attribute's name to the current input character, and its value to the empty string.
                    auto attribute = TagAttribute {
                        .name = { c },
                        .value =""
                    };

                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token)
                        {
                            m_current_attribute = &token.attributes.emplace_back(std::move(attribute));
                        },
                        [&](EndTagToken& token)
                        {
                            m_current_attribute = &token.attributes.emplace_back(std::move(attribute));
                        },
                        [](auto&&){ raise(SIGTRAP); }
                    }, m_current_token);

                    // Switch to the attribute name state.
                    m_state = State::AttributeName;
                    break;
                }

                // Anything else
                // Start a new attribute in the current tag token.
                // Set that attribute name and value to the empty string.
                auto attribute = TagAttribute {
                    .name = "",
                    .value =""
                };

                std::visit(kori::VariantOverloadSet {
                    [&](StartTagToken& token)
                    {
                        m_current_attribute = &token.attributes.emplace_back(std::move(attribute));
                    },
                    [&](EndTagToken& token)
                    {
                        m_current_attribute = &token.attributes.emplace_back(std::move(attribute));
                    },
                    [](auto&&){ raise(SIGTRAP); }
                }, m_current_token);

                // Reconsume in the attribute name state.
                reconsume_in(State::AttributeName);
                break;
            }
            case State::AttributeName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // Reconsume in the after attribute name state.
                    reconsume_in(State::AfterAttributeName);
                    break;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                // U+002F SOLIDUS (/)
                // U+003E GREATER-THAN SIGN (>)
                // U+003D EQUALS SIGN (=)
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ' || c == '/' || c == '>' || c == '=')
                {
                    // Switch to the before attribute value state.
                    m_state = State::BeforeAttributeValue;
                    break;
                }

                // ASCII upper alpha
                if (is_ascii_upper_alpha(c))
                {
                    // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current attribute's name.
                    m_current_attribute->name += static_cast<char>(std::tolower(c));
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the current attribute's name.
                    m_current_attribute->name += "�";
                    break;
                }

                // U+0022 QUOTATION MARK (")
                // U+0027 APOSTROPHE (')
                // U+003C LESS-THAN SIGN (<)
                if (c == '"' || c == '\'' || c == '<')
                {
                    // This is an unexpected-character-in-attribute-name parse error.
                    // parse_error(ErrorType::UnexpectedCharacterInAttributeName);
                    // Treat it as per the "anything else" entry below.
                }

                // Anything else
                // Append the current input character to the current attribute's name.
                m_current_attribute->name += c;
                break;
            }
            case State::BeforeAttributeValue:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Ignore the character.
                    break;
                }

                // U+0022 QUOTATION MARK (")
                if (c == '"')
                {
                    // Switch to the attribute value (double-quoted) state.
                    m_state = State::AttributeValueDoubleQuoted;
                    break;
                }

                // U+0027 APOSTROPHE (')
                if (c == '\'')
                {
                    // Switch to the attribute value (single-quoted) state.
                    m_state = State::AttributeValueSingleQuoted;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // This is a missing-attribute-value parse error.
                    // parse_error(ErrorType::MissingAttributeValue);

                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current tag token.
                    emit_token(m_current_token);
                    break;
                }

                // Anything else
                // Reconsume in the attribute value (unquoted) state.
                reconsume_in(State::AttributeValueUnquoted);
                break;
            }
            case State::AttributeValueDoubleQuoted:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-tag parse error.
                    // parse_error(ErrorType::EOFInTag);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0022 QUOTATION MARK (")
                if (c == '"')
                {
                    // Switch to the after attribute value (quoted) state.
                    m_state = State::AfterAttributeValueQuoted;
                    break;
                }

                // U+0026 AMPERSAND (&)
                if (c == '&')
                {
                    // Set the return state to the attribute value (double-quoted) state.
                    m_return_state = State::AttributeValueDoubleQuoted;

                    // Switch to the character reference state.
                    m_state = State::CharacterReference;
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the current attribute's value.
                    m_current_attribute->value += "�";
                    break;
                }

                // Anything else
                // Append the current input character to the current attribute's value.
                m_current_attribute->value += c;
                break;
            }
            case State::AttributeValueUnquoted:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-tag parse error.
                    // parse_error(ErrorType::EOFInTag);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Switch to the before attribute name state.
                    m_state = State::BeforeAttributeName;
                    break;
                }

                // U+0026 AMPERSAND (&)
                if (c == '&')
                {
                    // Set the return state to the attribute value (unquoted) state.
                    m_return_state = State::AttributeValueUnquoted;

                    // Switch to the character reference state.
                    m_state = State::CharacterReference;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current tag token.
                    emit_token(m_current_token);
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the current attribute's value.
                    m_current_attribute->value += "�";
                    break;
                }

                // U+0022 QUOTATION MARK (")
                // U+0027 APOSTROPHE (')
                // U+003C LESS-THAN SIGN (<)
                // U+003D EQUALS SIGN (=)
                // U+0060 GRAVE ACCENT (`)
                if (c == '"' || c == '\'' || c == '<' || c == '=' || c == '`')
                {
                    // This is an unexpected-character-in-unquoted-attribute-value parse error.
                    // parse_error(ErrorType::UnexpectedCharacterInUnquotedAttributeValue);

                    // Treat it as per the "anything else" entry below.
                }

                // Anything else
                // Append the current input character to the current attribute's value.
                m_current_attribute->value += c;
                break;
            }
            case State::AfterAttributeValueQuoted:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-tag parse error.
                    // parse_error(ErrorType::EOFInTag);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // Switch to the before attribute name state.
                    m_state = State::BeforeAttributeName;
                    break;
                }

                // U+002F SOLIDUS (/)
                if (c == '/')
                {
                    // Switch to the self-closing start tag state.
                    m_state = State::SelfClosingStartTag;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current tag token.
                    emit_token(m_current_token);
                    break;
                }

                // Anything else
                // This is a missing-whitespace-between-attributes parse error.
                // parse_error(ErrorType::MissingWhitespaceBetweenAttributes);

                // Reconsume in the before attribute name state.
                reconsume_in(State::BeforeAttributeName);
                break;
            }
            case State::CommentStart:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // U+002D HYPHEN-MINUS (-)
                if (c == '-')
                {
                    // Switch to the comment start dash state.
                    m_state = State::CommentStartDash;
                    break;
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // This is an abrupt-closing-of-empty-comment parse error.
                    // parse_error(ErrorType::AbruptClosingOfEmptyComment);

                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current comment token.
                    emit_token(m_current_token);
                    break;
                }

                // Anything else
                // Reconsume in the comment state.
                reconsume_in(State::Comment);
                break;
            }
            case State::Comment:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-comment parse error.
                    // parse_error(ErrorType::EOFInComment);

                    // Emit the current comment token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+003C LESS-THAN SIGN (<)
                if (c == '<')
                {
                    // Append the current input character to the comment token's data.
                    std::get<CommentToken>(m_current_token).data += c;

                    // Switch to the comment less-than sign state.
                    m_state = State::CommentLessThanSign;
                    break;
                }

                // U+002D HYPHEN-MINUS (-)
                if (c == '-')
                {
                    // Switch to the comment end dash state.
                    m_state = State::CommentEndDash;
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // Append a U+FFFD REPLACEMENT CHARACTER character to the comment token's data.
                    std::get<CommentToken>(m_current_token).data += "�";
                    break;
                }

                // Anything else
                // Append the current input character to the comment token's data.
                std::get<CommentToken>(m_current_token).data += c;
                break;
            }
            case State::CommentEndDash:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-comment parse error.
                    // parse_error(ErrorType::EOFInComment);

                    // Emit the current comment token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }

                // U+002D HYPHEN-MINUS (-)
                if (c == '-')
                {
                    // Switch to the comment end state.
                    m_state = State::CommentEnd;
                    break;
                }

                // Anything else
                // Append a U+002D HYPHEN-MINUS character (-) to the comment token's data.
                std::get<CommentToken>(m_current_token).data += '-';

                // Reconsume in the comment state.
                reconsume_in(State::Comment);
                break;
            }
            case State::CommentEnd:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // This is an eof-in-comment parse error.
                    // parse_error(ErrorType::EOFInComment);

                    // Emit the current comment token.
                    emit_token(m_current_token);

                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    return ProcessResult::Abort;
                }
                
                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // Switch to the data state.
                    m_state = State::Data;

                    // Emit the current comment token.
                    emit_token(m_current_token);
                    break;
                }

                // U+0021 EXCLAMATION MARK (!)
                if (c == '!')
                {
                    // Switch to the comment end bang state.
                    m_state = State::CommentEndBang;
                    break;
                }

                // U+002D HYPHEN-MINUS (-)
                if (c == '-')
                {
                    // Append a U+002D HYPHEN-MINUS character (-) to the comment token's data.
                    std::get<CommentToken>(m_current_token).data += '-';
                    break;
                }

                // Anything else
                // Append two U+002D HYPHEN-MINUS characters (-) to the comment token's data.
                std::get<CommentToken>(m_current_token).data += "--";

                // Reconsume in the comment state.
                reconsume_in(State::Comment);
                break;
            }
            case State::CommentLessThanSign:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // U+0021 EXCLAMATION MARK (!)
                if (c == '!')
                {
                    // Append the current input character to the comment token's data.
                    std::get<CommentToken>(m_current_token).data += c;

                    // Switch to the comment less-than sign bang state.
                    m_state = State::CommentLessThanSignBang;
                    break;
                }

                // U+003C LESS-THAN SIGN (<)
                if (c == '<')
                {
                    // Append the current input character to the comment token's data.
                    std::get<CommentToken>(m_current_token).data += c;
                    break;
                }

                // Anything else
                // Reconsume in the comment state.
                reconsume_in(State::Comment);
                break;
            }
            case State::RCDATA:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // EOF
                if (reached_eof())
                {
                    // Emit an end-of-file token.
                    emit_token(EOFToken{});
                    break;
                }

                // U+0026 AMPERSAND (&)
                if (c == '&')
                {
                    // Set the return state to the RCDATA state. Switch to the character reference state.
                    m_return_state = State::RCDATA;
                    m_state = State::CharacterReference;
                    break;
                }

                // U+003C LESS-THAN SIGN (<)
                if (c == '<')
                {
                    // Switch to the RCDATA less-than sign state.
                    m_state = State::RCDATALessThanSign;
                    break;
                }

                // U+0000 NULL
                if (c == '\0')
                {
                    // This is an unexpected-null-character parse error.
                    // parse_error(ErrorType::UnexpectedNullCharacter);

                    // FIXME(Peter): Handle multi-byte characters
                    // Emit a U+FFFD REPLACEMENT CHARACTER character token.
                    emit_token(CharacterToken{ '?' });
                    break;
                }

                // Anything else
                //     Emit the current input character as a character token.
                emit_token(CharacterToken{ c });
                break;
            }
            case State::RCDATALessThanSign:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // U+002F SOLIDUS (/)
                if (c == '/')
                {
                    // Set the temporary buffer to the empty string.
                    m_temporary_buffer = "";

                    // Switch to the RCDATA end tag open state.
                    m_state = State::RCDATAEndTagOpen;
                    break;
                }

                // Anything else
                //     Emit a U+003C LESS-THAN SIGN character token.
                emit_token(CharacterToken{ '<' });

                // Reconsume in the RCDATA state.
                reconsume_in(State::RCDATA);
                break;
            }
            case State::RCDATAEndTagOpen:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // ASCII alpha
                if (is_ascii_alpha(c))
                {
                    // Create a new end tag token, set its tag name to the empty string.
                    m_current_token = EndTagToken { "" };

                    // Reconsume in the RCDATA end tag name state.
                    reconsume_in(State::RCDATAEndTagName);
                    break;
                }

                // Anything else
                //     Emit a U+003C LESS-THAN SIGN character token and a U+002F SOLIDUS character token. Reconsume in the RCDATA state.
                emit_token(CharacterToken{ '<' });
                emit_token(CharacterToken{ '/' });
                reconsume_in(State::RCDATA);
                break;
            }
            case State::RCDATAEndTagName:
            {
                // Consume the next input character:
                const char c = consume_next_character();

                // U+0009 CHARACTER TABULATION (tab)
                // U+000A LINE FEED (LF)
                // U+000C FORM FEED (FF)
                // U+0020 SPACE
                if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                {
                    // If the current end tag token is an appropriate end tag token, then switch to the before attribute name state.
                    if (current_is_appropriate_end_tag())
                    {
                        m_state = State::BeforeAttributeName;
                        break;
                    }

                    // Otherwise, treat it as per the "anything else" entry below.
                }

                // U+002F SOLIDUS (/)
                if (c == '/')
                {
                    // If the current end tag token is an appropriate end tag token, then switch to the self-closing start tag state.
                    if (current_is_appropriate_end_tag())
                    {
                        m_state = State::SelfClosingStartTag;
                        break;
                    }

                    // Otherwise, treat it as per the "anything else" entry below.
                }

                // U+003E GREATER-THAN SIGN (>)
                if (c == '>')
                {
                    // If the current end tag token is an appropriate end tag token, then switch to the data state and emit the current tag token.
                    if (current_is_appropriate_end_tag())
                    {
                        m_state = State::Data;
                        emit_token(m_current_token);
                        break;
                    }

                    // Otherwise, treat it as per the "anything else" entry below.
                }

                // ASCII upper alpha
                if (is_ascii_upper_alpha(c))
                {
                    // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current tag token's tag name.
                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token) { token.name += static_cast<char>(std::tolower(c)); },
                        [&](EndTagToken& token) { token.name += static_cast<char>(std::tolower(c)); },
                        [](auto&&) { raise(SIGTRAP); }
                    }, m_current_token);

                    // Append the current input character to the temporary buffer.
                    m_temporary_buffer += c;
                    break;
                }

                // ASCII lower alpha
                if (is_ascii_lower_alpha(c))
                {
                    // Append the current input character to the current tag token's tag name.
                    std::visit(kori::VariantOverloadSet {
                        [&](StartTagToken& token) { token.name += c; },
                        [&](EndTagToken& token) { token.name += c; },
                        [](auto&&) { raise(SIGTRAP); }
                    }, m_current_token);

                    // Append the current input character to the temporary buffer.
                    m_temporary_buffer += c;
                    break;
                }

                // Anything else
                // Emit a U+003C LESS-THAN SIGN character token,
                emit_token(CharacterToken{ '<'} );

                // a U+002F SOLIDUS character token,
                emit_token(CharacterToken{ '/'} );

                // and a character token for each of the characters in the temporary buffer (in the order they were added to the buffer).
                for (const auto character : m_temporary_buffer)
                {
                    emit_token(CharacterToken{ character } );
                }

                // Reconsume in the RCDATA state.
                reconsume_in(State::RCDATA);
                break;
            }
            default:
            {
                raise(SIGTRAP);
                //MWL_VERIFY(false, "Unimplemented state");
                break;
            }
        }

        return ProcessResult::Continue;
    }

}
