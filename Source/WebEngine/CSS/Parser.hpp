#pragma once

#include "Parser.hpp"
#include "Tokenizer.hpp"
#include "ObjectModel/Rules.hpp"

#include "Kori/Core.hpp"

namespace Hanami::CSS {

    struct SimpleBlockToken
    {
        std::variant<OpenSquareBracketToken, OpenParenthesesToken, OpenCurlyBracketToken> value;

        SimpleBlockToken(const Token& token)
        {
            std::visit(Kori::VariantOverloadSet {
                [&](const OpenSquareBracketToken& t) { value = t; },
                [&](const OpenParenthesesToken& t) { value = t; },
                [&](const OpenCurlyBracketToken& t) { value = t; },
                [](auto&&) { HANAMI_TRAP(); },
            }, token);
        }
    };

    struct ComponentValue;

    struct SimpleBlock
    {
        SimpleBlockToken token;
        std::vector<ComponentValue> value;
    };

    // https://www.w3.org/TR/css-syntax-3/#component-value
    struct ComponentValue
    {
        using ValueType = std::variant<
            Token,
            // Function,
            SimpleBlock
        >;

        ValueType value;

        ComponentValue(const ValueType& value)
            : value(value)
        {
            // TODO(Peter): If Token, verify that it's a preserved token
        }
    };

    struct QualifiedRule
    {
        std::vector<ComponentValue> prelude;
        SimpleBlock block;

        QualifiedRule()
            : block({ .token = SimpleBlockToken(OpenCurlyBracketToken{}), .value = {} })
        {}
    };

    using RuleVariant = std::variant<QualifiedRule>;

    struct ParserSettings
    {
        bool dump_tokens = false;
        bool dump_ruleset = false;
    };
    
    class Parser
    {
    public:
        static auto parse_stylesheet_from_file(const std::filesystem::path& path, ParserSettings settings = {}) -> StyleSheet;

    private:
        auto parse_stylesheet(ParserSettings settings) -> StyleSheet;

        auto consume_next_input_token() -> const Token&;
        auto current_input_token() -> const Token&;

        auto consume_list_of_rules(bool top_level) -> std::vector<RuleVariant>;

        auto consume_component_value() -> ComponentValue;
        auto consume_qualified_rule() -> std::optional<QualifiedRule>;
        auto consume_simple_block() -> SimpleBlock;

    private:
        std::vector<Token> m_tokens;
        size_t m_next_input_token = 0;
    };

}
