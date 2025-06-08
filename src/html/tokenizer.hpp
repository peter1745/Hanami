#pragma once

#include <span>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <string_view>

namespace hanami::html {

    struct DOCTYPEToken
    {
        std::string name{};
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

    struct Tokenizer
    {
        // https://html.spec.whatwg.org/multipage/parsing.html#tokenization
        auto tokenize(std::string_view input) -> std::vector<Token>;
    };

}
