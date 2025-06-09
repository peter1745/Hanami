#pragma once

#include <span>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
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

    struct Tokenizer
    {
        // https://html.spec.whatwg.org/multipage/parsing.html#tokenization
        auto tokenize(std::string_view input) -> std::vector<Token>;
    };

}
