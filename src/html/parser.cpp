#include "parser.hpp"

#include <regex>
#include <print>
#include <algorithm>

#include "kori/core.hpp"

namespace hanami::html {

    void print_token(const Token& t)
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
    
    void Parser::parse(std::string_view html)
    {
        m_input_stream = normalize_input_stream(html);

        m_tokenizer.start(m_input_stream, [&](const Token& token)
        {
            print_token(token);
            //m_tree_builder.process_token(token);
        });
    }

    // https://infra.spec.whatwg.org/#normalize-newlines
    auto Parser::normalize_input_stream(std::string_view in) noexcept -> std::string
    {
        auto str = std::regex_replace(std::string{ in }, std::regex("\r\n"), "\n");
        std::ranges::replace(str, '\r', '\n');
        return str;
    }

}
