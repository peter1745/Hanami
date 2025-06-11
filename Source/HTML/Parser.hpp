#pragma once

#include "Tokenizer.hpp"
#include "TreeBuilder.hpp"

namespace Hanami::HTML {

    class Parser
    {
    public:
        Parser() noexcept;

        void parse(std::string_view html);

        auto document() const -> DOM::Document*;

    private:
        [[nodiscard]]
        static auto normalize_input_stream(std::string_view in) noexcept -> std::string;

    private:
        std::string m_input_stream;
        Tokenizer m_tokenizer;
        TreeBuilder m_tree_builder;
    };

}
