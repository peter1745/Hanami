#include "Parser.hpp"

#include <regex>
#include <print>
#include <algorithm>

namespace Hanami::HTML {

    Parser::Parser() noexcept
        : m_tree_builder(&m_tokenizer)
    {
    }

    void Parser::parse(std::string_view html)
    {
        m_input_stream = normalize_input_stream(html);

        m_tokenizer.start(m_input_stream, [&](const Token& token)
        {
            m_tree_builder.process_token(token);
        });
    }

    auto Parser::document() const -> DOM::Document*
    {
        return m_tree_builder.document();
    }

    // https://infra.spec.whatwg.org/#normalize-newlines
    auto Parser::normalize_input_stream(std::string_view in) noexcept -> std::string
    {
        auto str = std::regex_replace(std::string{ in }, std::regex("\r\n"), "\n");
        std::ranges::replace(str, '\r', '\n');
        return str;
    }

}
