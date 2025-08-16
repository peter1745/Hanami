#pragma once

#include "Tokenizer.hpp"

namespace Hanami::CSS {

    struct SimpleBlock;
    using ComponentValue = std::variant<Token, SimpleBlock>;

    struct SimpleBlock
    {
        Token token;
        std::vector<ComponentValue> value;
    };

    struct QualifiedRule
    {
        std::vector<ComponentValue> prelude;
        SimpleBlock block;
    };

    using Rule = std::variant<
        QualifiedRule
    >;

    struct ParserSettings
    {
        bool dump_tokens = false;
        bool dump_rules = false;
    };
    
    class Parser
    {
    public:
        Parser(ParserSettings settings);

        static void parse_from_file(const std::filesystem::path& filepath, ParserSettings settings);

    private:
        auto consume_next_token() -> Token*;

        void reconsume_current();

        // https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
        void parse_stylesheet();

        // https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-rules
        auto consume_rule_list(bool toplevel) -> std::vector<Rule>;

        // https://www.w3.org/TR/css-syntax-3/#consume-a-qualified-rule
        auto consume_qualified_rule(const Token& token) -> std::optional<QualifiedRule>;

        // https://www.w3.org/TR/css-syntax-3/#consume-a-component-value
        auto consume_component_value() -> ComponentValue;

        // https://www.w3.org/TR/css-syntax-3/#consume-a-simple-block
        auto consume_simple_block(size_t ending_token_idx) -> SimpleBlock;

    private:
        ParserSettings m_settings;
        std::vector<Token> m_tokens;
        size_t m_next_token_idx;
    };

}
