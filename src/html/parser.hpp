#pragma once

#include "tokenizer.hpp"
#include "tree_builder.hpp"

namespace hanami::html {

    class Parser
    {
    public:
        void parse(std::string_view html);

    private:
        [[nodiscard]]
        static auto normalize_input_stream(std::string_view in) noexcept -> std::string;

    private:
        std::string m_input_stream;
        Tokenizer m_tokenizer;
        TreeBuilder m_tree_builder;
    };

}
