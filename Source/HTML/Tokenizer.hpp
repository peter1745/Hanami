#pragma once

#include "Core/Core.hpp"
#include "Kori/Core.hpp"

#include <simdjson.h>

namespace Hanami::HTML {

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

    inline auto token_is_character(const Token& token, char c) -> bool
    {
        const auto* char_token = std::get_if<CharacterToken>(&token);
        return char_token && char_token->data == c;
    }

    inline auto token_is_start_tag(const Token& token, std::string_view tag) -> bool
    {
        const auto* tag_token = std::get_if<StartTagToken>(&token);
        return tag_token && tag_token->name == tag;
    }

    inline auto token_is_start_tag_any_of(const Token& token, std::span<const std::string_view> names) -> bool
    {
        if (!std::holds_alternative<StartTagToken>(token))
        {
            return false;
        }

        return std::ranges::any_of(names, [&](const std::string_view name)
        {
            return std::get<StartTagToken>(token).name == name;
        });
    }

    inline auto token_is_end_tag(const Token& token, std::string_view tag) -> bool
    {
        const auto* tag_token = std::get_if<EndTagToken>(&token);
        return tag_token && tag_token->name == tag;
    }

    inline auto token_is_end_tag_any_of(const Token& token, std::span<const std::string_view> names) -> bool
    {
        if (!std::holds_alternative<EndTagToken>(token))
        {
            return false;
        }

        return std::ranges::any_of(names, [&](const std::string_view name)
        {
            return std::get<EndTagToken>(token).name == name;
        });
    }

    inline auto token_tag_name(const Token& token) -> std::string_view
    {
        auto name = std::string_view{};
        std::visit(Kori::VariantOverloadSet {
            [&](const StartTagToken& start_tag) { name = start_tag.name; },
            [&](const EndTagToken& end_tag) { name = end_tag.name; },
            [](auto&&) {}
        }, token);
        return name;
    }

    class Tokenizer
    {
    public:
        Tokenizer();

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
            RCDATAEndTagOpen,
            RCDATAEndTagName,
            AmbiguousAmpersand,
            HexadecimalCharacterReferenceStart,
            DecimalCharacterReferenceStart,
            DecimalCharacterReference,
            NumericCharacterReferenceEnd,
        };

        void set_state(State state) noexcept
        {
            m_state = state;
        }

    private:
        void emit_token(const Token& token);

        auto consume_multiple_chars(size_t count) noexcept -> std::string_view;
        auto consume_next_character() noexcept -> char;
        void reconsume_in(State state) noexcept;

        [[nodiscard]]
        auto reached_eof() const noexcept -> bool;

        [[nodiscard]]
        auto next_characters_equals(char character, bool case_insensitive = false) const noexcept -> bool;

        [[nodiscard]]
        auto next_characters_equals(std::string_view chars, bool case_insensitive = false) const noexcept -> bool;

        enum class ProcessResult { Continue, Abort };
        auto process_next_token() -> ProcessResult;

        auto current_is_appropriate_end_tag() const noexcept -> bool;

        auto consumed_part_of_attribute() const noexcept -> bool;
        void flush_consumed_code_points();

    private:
        simdjson::ondemand::parser m_json_parser;
        simdjson::padded_string m_named_characters_str;
        simdjson::ondemand::document m_named_characters_lookup;
        uint32_t m_character_reference_code;

        EmitTokenFunc m_emit_token;
        std::string_view m_input_stream;

        State m_state = State::Invalid;
        State m_return_state = State::Invalid;

        std::string m_last_emitted_start_token_name;
        Token m_current_token{};

        size_t m_current_char_idx = 0;

        std::string m_temporary_buffer;

        TagAttribute* m_current_attribute = nullptr;
    };

}
