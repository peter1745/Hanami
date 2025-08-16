#include "Parser.hpp"

namespace Hanami::CSS {

    Parser::Parser(ParserSettings settings)
        : m_settings(std::move(settings))
    {
    }

    void Parser::parse_from_file(const std::filesystem::path& filepath, ParserSettings settings)
    {
        std::stringstream ss;
        auto stream = std::ifstream{ filepath };

        if (!stream)
        {
            std::println("Failed to open file");
        }

        ss << stream.rdbuf();

        auto tokens = Hanami::CSS::Tokenizer{}.run(ss.str());

        if (tokens.empty())
        {
            std::println("No tokens");
            return;
        }

        if (settings.dump_tokens)
        {
            int32_t indentation_level = 0;

            std::println("---------- Tokens ---------");
            for (const auto& token : tokens)
            {
                if (std::holds_alternative<LeftCurlyBracketToken>(token))
                {
                    std::println("LeftCurlyBracketToken");
                    ++indentation_level;
                    continue;
                }

                if (std::holds_alternative<RightCurlyBracketToken>(token))
                {
                    std::println("RightCurlyBracketToken");
                    --indentation_level;
                    continue;
                }

                for (int32_t i = 0; i < indentation_level; ++i)
                {
                    std::print("  ");
                }

                if (token_is<WhitespaceToken>(token))
                {
                    std::print("\r");
                }
                else
                {
                    print_token(token);
                }
            }
            std::println("---------------------------");
        }

        auto parser = Parser{ settings };
        parser.m_tokens = std::move(tokens);
        parser.parse_stylesheet();
    }

    auto Parser::consume_next_token() -> Token*
    {
        if (m_next_token_idx >= m_tokens.size())
        {
            return nullptr;
        }

        return &m_tokens[m_next_token_idx++];
    }

    void Parser::reconsume_current()
    {
        --m_next_token_idx;
    }

    // https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
    void Parser::parse_stylesheet()
    {
        // If input is a byte stream for stylesheet, decode bytes from input, and set input to the result.
        // Normalize input, and set input to the result.

        // Create a new stylesheet, with its location set to location (or null, if location was not passed).
        // Consume a list of rules from input, with the top-level flag set, and set the stylesheet’s value to the result.
        auto rules = consume_rule_list(true);

        if (m_settings.dump_rules)
        {
            auto print_component_value = [](const ComponentValue& value)
            {
                std::visit(Kori::VariantOverloadSet {
                    [&](const Token& token)
                    {
                        print_token(token);
                    },
                    [&](const SimpleBlock&)
                    {
                        HANAMI_NOT_IMPLEMENTED();
                    }
                }, value);
            };

            std::println();
            std::println("---------- Rules ----------");

            for (const auto& rule : rules)
            {
                std::println("Rule: {{");
                std::visit(Kori::VariantOverloadSet {
                    [&](const QualifiedRule& rule)
                    {
                        std::println("\tPrelude: {{");
                        for (const auto& part : rule.prelude)
                        {
                            std::print("\t\t");
                            print_component_value(part);
                        }
                        std::println("\t}}");

                        std::println("\tBlock: {{");

                        std::print("\t\tToken: ");
                        print_token(rule.block.token);

                        std::println("\t\tValue: {{");
                        for (const auto& value : rule.block.value)
                        {
                            std::print("\t\t\t");
                            print_component_value(value);
                        }
                        std::println("\t\t}}");

                        std::println("\t}}");
                    }
                }, rule);
                std::println("}}");
                std::println();
            }
            std::println("---------------------------");
        }

        // Return the stylesheet.
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-rules
    auto Parser::consume_rule_list(bool toplevel) -> std::vector<Rule>
    {
        // Create an initially empty list of rules.
        auto rules = std::vector<Rule>{};

        // Repeatedly consume the next input token:
        while (auto token = consume_next_token())
        {
            // <whitespace-token>
            if (std::holds_alternative<WhitespaceToken>(*token))
            {
                // Do nothing.
                continue;
            }

            // <EOF-token>
            if (std::holds_alternative<EOFToken>(*token))
            {
                // Return the list of rules.
                return rules;
            }

            // <CDO-token>
            // <CDC-token>
            if (!toplevel && (std::holds_alternative<CDOToken>(*token) || std::holds_alternative<CDCToken>(*token)))
            {
                // If the top-level flag is set, do nothing.
                // Otherwise, reconsume the current input token. Consume a qualified rule. If anything is returned, append it to the list of rules.
                HANAMI_NOT_IMPLEMENTED("CDOToken or CDCToken");
            }

            // <at-keyword-token>
            if (std::holds_alternative<AtKeywordToken>(*token))
            {
                // Reconsume the current input token. Consume an at-rule, and append the returned value to the list of rules.
                HANAMI_NOT_IMPLEMENTED("AtKeywordToken");
            }

            // anything else
            // Reconsume the current input token. Consume a qualified rule. If anything is returned, append it to the list of rules.
            reconsume_current();

            auto rule = consume_qualified_rule(*token);

            if (rule.has_value())
            {
                rules.emplace_back(std::move(rule.value()));
            }
        }

        return rules;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-qualified-rule
    auto Parser::consume_qualified_rule(const Token&) -> std::optional<QualifiedRule>
    {
        // Create a new qualified rule with its prelude initially set to an empty list, and its value initially set to nothing.
        auto rule = QualifiedRule{};

        // Repeatedly consume the next input token:
        while (auto token = consume_next_token())
        {
            // <EOF-token>
            if (token_is<EOFToken>(*token))
            {
                // This is a parse error. Return nothing.
                return {};
            }

            // <{-token>
            if (token_is<LeftCurlyBracketToken>(*token))
            {
                // Consume a simple block and assign it to the qualified rule’s block. Return the qualified rule.
                rule.block = consume_simple_block(Kori::variant_index<Token, RightCurlyBracketToken>());
                return rule;
            }

            // simple block with an associated token of <{-token>
            // Assign the block to the qualified rule’s block. Return the qualified rule.

            // anything else
            // Reconsume the current input token. Consume a component value. Append the returned value to the qualified rule’s prelude.
            reconsume_current();

            rule.prelude.emplace_back(consume_component_value());
        }

        return rule;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-component-value
    auto Parser::consume_component_value() -> ComponentValue
    {
        // Consume the next input token.
        auto token = consume_next_token();

        // If the current input token is a <{-token>, <[-token>, or <(-token>, consume a simple block and return it.
        if (token_is_any<LeftCurlyBracketToken, LeftSquareBracketToken, LeftParenthesesToken>(*token))
        {
            HANAMI_NOT_IMPLEMENTED();
        }

        // Otherwise, if the current input token is a <function-token>, consume a function and return it.
        if (token_is<FunctionToken>(*token))
        {
            HANAMI_NOT_IMPLEMENTED();
        }

        // Otherwise, return the current input token.
        return *token;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-simple-block
    auto Parser::consume_simple_block(size_t ending_token_idx) -> SimpleBlock
    {
        // Create a simple block with its associated token set to the current input token
        // and with its value initially set to an empty list.
        auto block = SimpleBlock {
           .token = m_tokens[m_next_token_idx - 1],
           .value = {}
        };

        // Repeatedly consume the next input token and process it as follows:
        while (auto token = consume_next_token())
        {
            // ending token
            if (token->index() == ending_token_idx)
            {
                // Return the block.
                return block;
            }

            // <EOF-token>
            if (token_is<EOFToken>(*token))
            {
                // This is a parse error. Return the block.
                return block;
            }

            // anything else
            // Reconsume the current input token.
            reconsume_current();

            // Consume a component value and append it to the value of the block.
            block.value.emplace_back(consume_component_value());
        }

        return block;
    }

}
