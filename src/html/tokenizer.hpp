#pragma once

#include <span>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace hanami::html {

    struct DOCTYPEToken
    {
        std::string name{};
        std::optional<std::string> public_identifier{ std::nullopt };
        std::optional<std::string> system_identifier{ std::nullopt };
        bool force_quirks{};
    };

    struct TagAttribute
    {
        std::string name;
        std::string value;
    };

    struct TagToken
    {
        std::string name{};
        bool self_closing{};
        std::vector<TagAttribute> attributes{};
    };
    struct StartTagToken : TagToken {};
    struct EndTagToken : TagToken {};

    inline auto get_token_attribute_value(const TagToken* token, std::string_view name) -> std::optional<std::string_view>
    {
        for (const auto& attrib : token->attributes)
        {
            if (attrib.name == name)
            {
                return attrib.value;
            }
        }

        return std::nullopt;
    }

    struct CommentToken
    {
        std::string data{};
    };

    struct CharacterToken
    {
        char data;
    };

    struct EOFToken {};

    using Token = std::variant<
        DOCTYPEToken,
        StartTagToken,
        EndTagToken,
        CommentToken,
        CharacterToken,
        EOFToken
    >;

    template<typename T>
    auto token_is(const Token& token) noexcept -> bool
    {
        return std::holds_alternative<T>(token);
    }

    class Tokenizer
    {
    public:
        using EmitTokenFunc = std::function<void(const Token&)>;
        void start(std::string_view input, EmitTokenFunc func);

        static void print_token(const Token& t);

    public:
        enum class State
        {
            Invalid = -1,

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
            RAWTEXT,
            RCDATA,
            RCDATALessThanSign,
        };

        void set_state(State state) noexcept
        {
            m_state = state;
        }

    private:
        void emit_token(const Token& token) const;

        auto consume_multiple_chars(size_t count) noexcept -> std::string_view;
        auto consume_next_character() noexcept -> char;
        void reconsume_in(State state) noexcept;

        [[nodiscard]]
        auto reached_eof() const noexcept -> bool;

        [[nodiscard]]
        auto next_characters_equals(std::string_view chars, bool case_insensitive = false) const noexcept -> bool;

        enum class ProcessResult { Continue, Abort };
        auto process_next_token() -> ProcessResult;

    private:
        EmitTokenFunc m_emit_token;
        std::string_view m_input_stream;

        State m_state = State::Invalid;
        State m_return_state = State::Invalid;

        Token m_current_token{};

        size_t m_current_char_idx = 0;

        std::string m_temporary_buffer;

        TagAttribute* m_current_attribute = nullptr;
    };

}
