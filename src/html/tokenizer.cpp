#include "tokenizer.hpp"

#include <kori/core.hpp>

#include <regex>
#include <print>
#include <algorithm>
#include <cstring>
#include <signal.h>

namespace hanami::html {

    enum class TokenizerState
    {
        Data,
        CharacterReference,
        TagOpen,
        NamedCharacterReference,
        NumericCharacterReference,
        MarkupDeclarationOpen,
        EndTagOpen,
        TagName,
        BogusComment,
        CommentStart,
        DOCTYPE,
        BeforeDOCTYPEName,
        DOCTYPEName,
        AfterDOCTYPEName,
        BeforeAttributeName,
        SelfClosingStartTag,
        AfterAttributeName,
        AttributeName,
        BeforeAttributeValue,
        AttributeValueDoubleQuoted,
        AttributeValueSingleQuoted,
        AttributeValueUnquoted,
        AfterAttributeValueQuoted,
        CommentStartDash,
        Comment,
        CommentLessThanSign,
        CommentEndDash,
        CommentEnd,
        CommentEndBang,
        CommentLessThanSignBang,
    };

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

    static auto equals_case_insensitive(std::string_view a, std::string_view b) -> bool
    {
        if (a.length() != b.length())
        {
            return false;
        }

        for (size_t i = 0; i < a.length(); ++i)
        {
            if (std::tolower(a[i]) != std::tolower(b[i]))
            {
                return false;
            }
        }

        return true;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#tokenization
    auto Tokenizer::tokenize(std::string_view input) -> std::vector<Token>
    {
        auto result = std::vector<Token>{};
        auto source = std::string{ input };

        // https://infra.spec.whatwg.org/#normalize-newlines
        source = std::regex_replace(std::string{ input }, std::regex("\r\n"), "\n");
        std::ranges::replace(source, '\r', '\n');

        // Tokenization
        auto next_char = source.begin();
        auto current_char = std::string::iterator{};

        auto state = TokenizerState::Data;
        auto return_state = TokenizerState::Data;
        (void)return_state;

        auto consume_next_character = [&]
        {
            current_char = next_char;

            if (current_char == source.end())
            {
                return current_char;
            }

            ++next_char;
            return current_char;
        };

        auto consume_n_characters = [&](size_t n)
        {
            for (size_t i = 0; i < n; ++i)
            {
                (void)consume_next_character();
            }
        };

        auto inspect_chars_forward = [&](size_t n) -> std::string_view
        {
            return { next_char, next_char + n };
        };

        auto emit_eof = [&]
        {
            result.emplace_back(EOFToken{});
        };

        auto temporary_buffer = std::string{};
        auto current_token = Token{};
        TagAttribute* current_attribute = nullptr;

        while (current_char != source.end())
        {
            switch (state)
            {
                case TokenizerState::Data:
                {
                    // Consume the next input character
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    if (c == '&') // U+0026 AMPERSAND (&)
                    {
                        // Set the return state to the data state.
                        return_state = TokenizerState::Data;

                        // Switch to the character reference state.
                        state = TokenizerState::CharacterReference;
                        break;
                    }

                    if (c == '<') // U+003C LESS-THAN SIGN (<)
                    {
                        // Switch to the tag open state.
                        state = TokenizerState::TagOpen;
                        break;
                    }

                    if (c == '\0') // U+0000 NULL
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Emit the current input character as a character token.
                        result.emplace_back(CharacterToken{ c });
                        break;
                    }

                    // Anything else
                    // Emit the current input character as a character token.
                    result.emplace_back(CharacterToken{ c });
                    break;
                }
                case TokenizerState::TagOpen:
                {
                    // Consume the next input character
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-before-tag-name parse error.
                        // parse_error(ErrorType::EOFBeforeTagName);

                        // Emit a U+003C LESS-THAN SIGN character token and an end-of-file token.
                        result.emplace_back(CharacterToken{ '<' });
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    if (c == '!') // U+0021 EXCLAMATION MARK (!)
                    {
                        // Switch to the markup declaration open state.
                        state = TokenizerState::MarkupDeclarationOpen;
                        break;
                    }

                    if (c == '/') // U+002F SOLIDUS (/)
                    {
                        // Switch to the end tag open state.
                        state = TokenizerState::EndTagOpen;
                        break;
                    }

                    if (is_ascii_alpha(c)) // ASCII alpha
                    {
                        // Create a new start tag token, set its tag name to the empty string.
                        current_token = StartTagToken{ "" };

                        // Reconsume in the tag name state.
                        --next_char;
                        state = TokenizerState::TagName;
                        break;
                    }

                    if (c == '?') // U+003F QUESTION MARK (?)
                    {
                        // This is an unexpected-question-mark-instead-of-tag-name parse error.
                        // parse_error(ErrorType::UnexpectedQuestionMarkInsteadOfTagName);

                        // Create a comment token whose data is the empty string.
                        current_token = CommentToken{ "" };

                        // Reconsume in the bogus comment state.
                        --next_char;
                        state = TokenizerState::BogusComment;
                        break;
                    }

                    // Anything else
                    // This is an invalid-first-character-of-tag-name parse error.
                    // parse_error(ErrorType::InvalidFirstCharacterOfTagName);

                    // Emit a U+003C LESS-THAN SIGN character token.
                    result.emplace_back(CharacterToken{ '<' });

                    // Reconsume in the data state.
                    --next_char;
                    state = TokenizerState::Data;
                    break;
                }
                case TokenizerState::EndTagOpen:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-before-tag-name parse error.
                        // parse_error(ErrorType::EOFBeforeTagName);

                        // Emit a U+003C LESS-THAN SIGN character token, a U+002F SOLIDUS character token and an end-of-file token.
                        result.emplace_back(CharacterToken{ '<' });
                        result.emplace_back(CharacterToken{ '/' });
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // ASCII alpha
                    if (is_ascii_alpha(c))
                    {
                        // Create a new end tag token, set its tag name to the empty string.
                        current_token = EndTagToken { "" };

                        // Reconsume in the tag name state.
                        --next_char;
                        state = TokenizerState::TagName;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // This is a missing-end-tag-name parse error.
                        // parse_error(ErrorType::MissingEndTagName);

                        // Switch to the data state.
                        state = TokenizerState::Data;
                        break;
                    }

                    // Anything else
                    // This is an invalid-first-character-of-tag-name parse error.
                    // parse_error(ErrorType::InvalidFirstCharacterOfTagName);

                    // Create a comment token whose data is the empty string.
                    current_token = CommentToken{ "" };

                    // Reconsume in the bogus comment state.
                    state = TokenizerState::BogusComment;
                    break;
                }
                case TokenizerState::MarkupDeclarationOpen:
                {
                    // If the next few characters are:
                    // Two U+002D HYPHEN-MINUS characters (-)
                    if (inspect_chars_forward(2) == "--")
                    {
                        // Consume those two characters
                        consume_n_characters(2);

                        // Create a comment token whose data is the empty string
                        current_token = CommentToken{ "" };

                        // Switch to the comment start state.
                        state = TokenizerState::CommentStart;
                        break;
                    }

                    // ASCII case-insensitive match for the word "DOCTYPE"
                    auto doctype_length = std::strlen("DOCTYPE");
                    if (equals_case_insensitive(inspect_chars_forward(doctype_length), "DOCTYPE"))
                    {
                        // Consume those characters
                        consume_n_characters(doctype_length);

                        // Switch to the DOCTYPE state.
                        state = TokenizerState::DOCTYPE;
                        break;
                    }

                    // The string "[CDATA[" (the five uppercase letters "CDATA" with a U+005B LEFT SQUARE BRACKET character before and after)
                    auto cdata_length = std::strlen("[CDATA[");
                    if (inspect_chars_forward(cdata_length) == "[CDATA[")
                    {
                        // Consume those characters.
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
                    current_token = CommentToken{ "" };

                    // Switch to the bogus comment state (don't consume anything in the current state).
                    state = TokenizerState::BogusComment;
                    break;
                }
                case TokenizerState::DOCTYPE:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-doctype parse error.
                        // parse_error(ErrorType::EOFInDOCTYPE);

                        // Create a new DOCTYPE token.
                        // Set its force-quirks flag to on.
                        current_token = DOCTYPEToken {
                            .force_quirks = true
                        };

                        // Emit the current token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+0009 CHARACTER TABULATION (tab)
                    // U+000A LINE FEED (LF)
                    // U+000C FORM FEED (FF)
                    // U+0020 SPACE
                    if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                    {
                        // Switch to the before DOCTYPE name state.
                        state = TokenizerState::BeforeDOCTYPEName;
                        break;
                    }

                    if (c == '>') // U+003E GREATER-THAN SIGN (>)
                    {
                        // Reconsume in the before DOCTYPE name state.
                        --next_char;
                        state = TokenizerState::BeforeDOCTYPEName;
                        break;
                    }

                    // Anything else
                    // This is a missing-whitespace-before-doctype-name parse error.
                    // parse_error(ErrorType::MissingWhitespaceBeforeDOCTYPEName);

                    // Reconsume in the before DOCTYPE name state.
                    --next_char;
                    state = TokenizerState::BeforeDOCTYPEName;
                    break;
                }
                case TokenizerState::BeforeDOCTYPEName:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-doctype parse error.
                        // parse_error(ErrorType::EOFInDOCTYPE);

                        // Create a new DOCTYPE token.
                        // Set its force-quirks flag to on.
                        current_token = DOCTYPEToken {
                            .force_quirks = true
                        };

                        // Emit the current token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

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
                        current_token = DOCTYPEToken {
                            .name = std::string{ static_cast<char>(std::tolower(c)) }
                        };

                        // Switch to the DOCTYPE name state.
                        state = TokenizerState::DOCTYPEName;
                        break;
                    }

                    // U+0000 NULL
                    if (c == '\0')
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Create a new DOCTYPE token.
                        // Set the token's name to a U+FFFD REPLACEMENT CHARACTER character.
                        current_token = DOCTYPEToken {
                            .name = "�"
                        };

                        // Switch to the DOCTYPE name state.
                        state = TokenizerState::DOCTYPEName;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // This is a missing-doctype-name parse error.
                        // parse_error(ErrorType::MissingDOCTYPEName);

                        // Create a new DOCTYPE token.
                        // Set its force-quirks flag to on.
                        current_token = DOCTYPEToken { .force_quirks = true };

                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // Anything else
                    // Create a new DOCTYPE token.
                    // Set the token's name to the current input character.
                    current_token = DOCTYPEToken { .name = { c } };

                    // Switch to the DOCTYPE name state.
                    state = TokenizerState::DOCTYPEName;
                    break;
                }
                case TokenizerState::DOCTYPEName:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-doctype parse error.
                        // parse_error(ErrorType::EOFInDOCTYPE);

                        // Set the current DOCTYPE token's force-quirks flag to on.
                        std::get<DOCTYPEToken>(current_token).force_quirks = true;

                        // Emit the current DOCTYPE token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+0009 CHARACTER TABULATION (tab)
                    // U+000A LINE FEED (LF)
                    // U+000C FORM FEED (FF)
                    // U+0020 SPACE
                    if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                    {
                        // Switch to the after DOCTYPE name state.
                        state = TokenizerState::AfterDOCTYPEName;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current DOCTYPE token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // ASCII upper alpha
                    if (is_ascii_upper_alpha(c))
                    {
                        // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current DOCTYPE token's name.
                        std::get<DOCTYPEToken>(current_token).name += static_cast<char>(std::tolower(c));
                        break;
                    }

                    // U+0000 NULL
                    if (c == '\0')
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Append a U+FFFD REPLACEMENT CHARACTER character to the current DOCTYPE token's name.
                        std::get<DOCTYPEToken>(current_token).name += "�";
                        break;
                    }

                    // Anything else
                    // Append the current input character to the current DOCTYPE token's name.
                    std::get<DOCTYPEToken>(current_token).name += c;
                    break;
                }
                case TokenizerState::CharacterReference:
                {
                    // Set the temporary buffer to the empty string.
                    temporary_buffer = "";

                    // Append a U+0026 AMPERSAND (&) character to the temporary buffer.
                    temporary_buffer.append("&");

                    // Consume the next input character
                    const auto c = *consume_next_character();

                    if (is_ascii_alpha_numeric(c)) // ASCII alphanumeric
                    {
                        // Reconsume in the named character reference state.
                        state = TokenizerState::NamedCharacterReference;
                        --next_char;
                        break;
                    }

                    if (c == '#') // U+0023 NUMBER SIGN (#)
                    {
                        // Append the current input character to the temporary buffer.
                        temporary_buffer += c;

                        // Switch to the numeric character reference state.
                        state = TokenizerState::NumericCharacterReference;
                        break;
                    }

                    // Anything else
                    // Flush code points consumed as a character reference.
                    for (auto character : temporary_buffer)
                    {
                        result.emplace_back(CharacterToken{ character });
                    }

                    // Reconsume in the return state.
                    state = return_state;
                    --next_char;
                    break;
                }
                case TokenizerState::TagName:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-tag parse error.
                        // parse_error(ErrorType::EOFInTag);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+0009 CHARACTER TABULATION (tab)
                    // U+000A LINE FEED (LF)
                    // U+000C FORM FEED (FF)
                    // U+0020 SPACE
                    if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                    {
                        // Switch to the before attribute name state.
                        state = TokenizerState::BeforeAttributeName;
                        break;
                    }

                    // U+002F SOLIDUS (/)
                    if (c == '/')
                    {
                        // Switch to the self-closing start tag state.
                        state = TokenizerState::SelfClosingStartTag;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current tag token.
                        result.emplace_back(current_token);
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
                        }, current_token);
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
                        }, current_token);
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
                    }, current_token);
                    break;
                }
                case TokenizerState::SelfClosingStartTag:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-tag parse error.
                        // parse_error(ErrorType::EOFInTag);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

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
                        }, current_token);

                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current tag token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // Anything else
                    // This is an unexpected-solidus-in-tag parse error.
                    // parse_error(ErrorType::UnexpectedSolidusInTag);

                    // Reconsume in the before attribute name state.
                    --next_char;
                    state = TokenizerState::BeforeAttributeName;
                    break;
                }
                case TokenizerState::BeforeAttributeName:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // Reconsume in the after attribute name state.
                        --next_char;
                        state = TokenizerState::AfterAttributeName;
                        break;
                    }

                    const auto c = *it;

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
                                current_attribute = &token.attributes.emplace_back(std::move(attribute));
                            },
                            [&](EndTagToken& token)
                            {
                                current_attribute = &token.attributes.emplace_back(std::move(attribute));
                            },
                            [](auto&&){ raise(SIGTRAP); }
                        }, current_token);

                        // Switch to the attribute name state.
                        state = TokenizerState::AttributeName;
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
                            current_attribute = &token.attributes.emplace_back(std::move(attribute));
                        },
                        [&](EndTagToken& token)
                        {
                            current_attribute = &token.attributes.emplace_back(std::move(attribute));
                        },
                        [](auto&&){ raise(SIGTRAP); }
                    }, current_token);

                    // Reconsume in the attribute name state.
                    --next_char;
                    state = TokenizerState::AttributeName;
                    break;
                }
                case TokenizerState::AttributeName:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // Reconsume in the after attribute name state.
                        --next_char;
                        state = TokenizerState::AfterAttributeName;
                        break;
                    }

                    const auto c = *it;

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
                        state = TokenizerState::BeforeAttributeValue;
                        break;
                    }

                    // ASCII upper alpha
                    if (is_ascii_upper_alpha(c))
                    {
                        // Append the lowercase version of the current input character (add 0x0020 to the character's code point) to the current attribute's name.
                        current_attribute->name += static_cast<char>(std::tolower(c));
                        break;
                    }

                    // U+0000 NULL
                    if (c == '\0')
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Append a U+FFFD REPLACEMENT CHARACTER character to the current attribute's name.
                        current_attribute->name += "�";
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
                    current_attribute->name += c;
                    break;
                }
                case TokenizerState::BeforeAttributeValue:
                {
                    // Consume the next input character:
                    const auto c = *consume_next_character();

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
                        state = TokenizerState::AttributeValueDoubleQuoted;
                        break;
                    }

                    // U+0027 APOSTROPHE (')
                    if (c == '\'')
                    {
                        // Switch to the attribute value (single-quoted) state.
                        state = TokenizerState::AttributeValueSingleQuoted;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // This is a missing-attribute-value parse error.
                        // parse_error(ErrorType::MissingAttributeValue);

                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current tag token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // Anything else
                    // Reconsume in the attribute value (unquoted) state.
                    --next_char;
                    state = TokenizerState::AttributeValueUnquoted;
                    break;
                }
                case TokenizerState::AttributeValueDoubleQuoted:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-tag parse error.
                        // parse_error(ErrorType::EOFInTag);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+0022 QUOTATION MARK (")
                    if (c == '"')
                    {
                        // Switch to the after attribute value (quoted) state.
                        state = TokenizerState::AfterAttributeValueQuoted;
                        break;
                    }

                    // U+0026 AMPERSAND (&)
                    if (c == '&')
                    {
                        // Set the return state to the attribute value (double-quoted) state.
                        return_state = TokenizerState::AttributeValueDoubleQuoted;

                        // Switch to the character reference state.
                        state = TokenizerState::CharacterReference;
                        break;
                    }

                    // U+0000 NULL
                    if (c == '\0')
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Append a U+FFFD REPLACEMENT CHARACTER character to the current attribute's value.
                        current_attribute->value += "�";
                        break;
                    }

                    // Anything else
                    // Append the current input character to the current attribute's value.
                    current_attribute->value += c;
                    break;
                }
                case TokenizerState::AfterAttributeValueQuoted:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    if (it == source.end()) // EOF
                    {
                        // This is an eof-in-tag parse error.
                        // parse_error(ErrorType::EOFInTag);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+0009 CHARACTER TABULATION (tab)
                    // U+000A LINE FEED (LF)
                    // U+000C FORM FEED (FF)
                    // U+0020 SPACE
                    if (c == '\t' || c == '\n' || c == '\f' || c == ' ')
                    {
                        // Switch to the before attribute name state.
                        state = TokenizerState::BeforeAttributeName;
                        break;
                    }

                    // U+002F SOLIDUS (/)
                    if (c == '/')
                    {
                        // Switch to the self-closing start tag state.
                        state = TokenizerState::SelfClosingStartTag;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current tag token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // Anything else
                    // This is a missing-whitespace-between-attributes parse error.
                    // parse_error(ErrorType::MissingWhitespaceBetweenAttributes);

                    // Reconsume in the before attribute name state.
                    --next_char;
                    state = TokenizerState::BeforeAttributeName;
                    break;
                }
                case TokenizerState::CommentStart:
                {
                    // Consume the next input character:
                    auto c = *consume_next_character();

                    // U+002D HYPHEN-MINUS (-)
                    if (c == '-')
                    {
                        // Switch to the comment start dash state.
                        state = TokenizerState::CommentStartDash;
                        break;
                    }

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // This is an abrupt-closing-of-empty-comment parse error.
                        // parse_error(ErrorType::AbruptClosingOfEmptyComment);

                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current comment token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // Anything else
                    // Reconsume in the comment state.
                    --next_char;
                    state = TokenizerState::Comment;
                    break;
                }
                case TokenizerState::Comment:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    // EOF
                    if (it == source.end())
                    {
                        // This is an eof-in-comment parse error.
                        // parse_error(ErrorType::EOFInComment);

                        // Emit the current comment token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+003C LESS-THAN SIGN (<)
                    if (c == '<')
                    {
                        // Append the current input character to the comment token's data.
                        std::get<CommentToken>(current_token).data += c;

                        // Switch to the comment less-than sign state.
                        state = TokenizerState::CommentLessThanSign;
                        break;
                    }

                    // U+002D HYPHEN-MINUS (-)
                    if (c == '-')
                    {
                        // Switch to the comment end dash state.
                        state = TokenizerState::CommentEndDash;
                        break;
                    }

                    // U+0000 NULL
                    if (c == '\0')
                    {
                        // This is an unexpected-null-character parse error.
                        // parse_error(ErrorType::UnexpectedNullCharacter);

                        // Append a U+FFFD REPLACEMENT CHARACTER character to the comment token's data.
                        std::get<CommentToken>(current_token).data += "�";
                        break;
                    }

                    // Anything else
                    // Append the current input character to the comment token's data.
                    std::get<CommentToken>(current_token).data += c;
                    break;
                }
                case TokenizerState::CommentEndDash:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    // EOF
                    if (it == source.end())
                    {
                        // This is an eof-in-comment parse error.
                        // parse_error(ErrorType::EOFInComment);

                        // Emit the current comment token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+002D HYPHEN-MINUS (-)
                    if (c == '-')
                    {
                        // Switch to the comment end state.
                        state = TokenizerState::CommentEnd;
                        break;
                    }

                    // Anything else
                    // Append a U+002D HYPHEN-MINUS character (-) to the comment token's data.
                    std::get<CommentToken>(current_token).data += '-';

                    // Reconsume in the comment state.
                    --next_char;
                    state = TokenizerState::Comment;
                    break;
                }
                case TokenizerState::CommentEnd:
                {
                    // Consume the next input character:
                    auto it = consume_next_character();

                    // EOF
                    if (it == source.end())
                    {
                        // This is an eof-in-comment parse error.
                        // parse_error(ErrorType::EOFInComment);

                        // Emit the current comment token.
                        result.emplace_back(current_token);

                        // Emit an end-of-file token.
                        emit_eof();
                        break;
                    }

                    const auto c = *it;

                    // U+003E GREATER-THAN SIGN (>)
                    if (c == '>')
                    {
                        // Switch to the data state.
                        state = TokenizerState::Data;

                        // Emit the current comment token.
                        result.emplace_back(current_token);
                        break;
                    }

                    // U+0021 EXCLAMATION MARK (!)
                    if (c == '!')
                    {
                        // Switch to the comment end bang state.
                        state = TokenizerState::CommentEndBang;
                        break;
                    }

                    // U+002D HYPHEN-MINUS (-)
                    if (c == '-')
                    {
                        // Append a U+002D HYPHEN-MINUS character (-) to the comment token's data.
                        std::get<CommentToken>(current_token).data += '-';
                        break;
                    }

                    // Anything else
                    // Append two U+002D HYPHEN-MINUS characters (-) to the comment token's data.
                    std::get<CommentToken>(current_token).data += "--";

                    // Reconsume in the comment state.
                    --next_char;
                    state = TokenizerState::Comment;
                    break;
                }
                case TokenizerState::CommentLessThanSign:
                {
                    // Consume the next input character:
                    const auto c = *consume_next_character();

                    // U+0021 EXCLAMATION MARK (!)
                    if (c == '!')
                    {
                        // Append the current input character to the comment token's data.
                        std::get<CommentToken>(current_token).data += c;

                        // Switch to the comment less-than sign bang state.
                        state = TokenizerState::CommentLessThanSignBang;
                        break;
                    }

                    // U+003C LESS-THAN SIGN (<)
                    if (c == '<')
                    {
                        // Append the current input character to the comment token's data.
                        std::get<CommentToken>(current_token).data += c;
                        break;
                    }

                    // Anything else
                    // Reconsume in the comment state.
                    --next_char;
                    state = TokenizerState::Comment;
                    break;
                }
                default:
                {
                    raise(SIGTRAP);
                    //MWL_VERIFY(false, "Unimplemented state");
                    break;
                }
            }
        }

        return result;
    }

}
