#include "Parser.hpp"

#include "Kori/Core.hpp"

#include <print>

namespace Hanami::CSS {

    void Parser::parse_stylesheet_from_file(const std::filesystem::path& path)
    {
        std::stringstream ss;
        std::ifstream stream(path);
        ss << stream.rdbuf();

        auto parser = Parser{};
        parser.m_tokens = Tokenizer{}.tokenize(ss.str());
        parser.parse_stylesheet();
    }

    static auto parse_selector(const std::vector<ComponentValue>& prelude) -> Selector
    {
        auto selector = Selector{};

        size_t next_part = 0;

        auto consume_next = [&] -> const Token&
        {
            return std::get<Token>(prelude[next_part++].value);
        };

        SimpleSelector* current_selector = nullptr;

        auto get_or_make_selector = [&]() -> SimpleSelector*
        {
            if (!current_selector)
            {
                auto& part = selector.parts.emplace_back(Combinator::None, SimpleSelector{});
                current_selector = &part.second;
            }

            return current_selector;
        };

        while (next_part < prelude.size())
        {
            const auto& value = consume_next();

            if (const auto* ident = std::get_if<IdentToken>(&value))
            {
                auto* s = get_or_make_selector();
                s->tag = ident->value;
                continue;
            }

            if (const auto* delim = std::get_if<DelimToken>(&value))
            {
                auto* s = get_or_make_selector();

                if (delim->value == '*')
                {
                    // Universal selector is an empty selector<
                    continue;
                }

                if (delim->value == '.')
                {
                    // Consume the next part (should be class name)
                    if (const auto* ident = std::get_if<IdentToken>(&consume_next()))
                    {
                        s->classes.emplace_back(ident->value);
                        continue;
                    }
                }
            }

            if (const auto* hash = std::get_if<HashToken>(&value))
            {
                auto* s = get_or_make_selector();
                s->id = hash->value;
            }
        }

        return selector;
    }

    void Parser::parse_stylesheet()
    {
        for (const auto& token : m_tokens)
        {
            print_token(token);
        }

        auto rules =  consume_list_of_rules(true);

        std::vector<Rule*> rule_objects;

        for (const auto& rule : rules)
        {
            std::visit(Kori::VariantOverloadSet {
                [&](const QualifiedRule& qr)
                {
                    auto* style_rule = new StyleRule(parse_selector(qr.prelude));
                    auto& style_declaration = style_rule->style_declaration();

                    for (size_t i = 0; i < qr.block.value.size(); i += 4)
                    {
                        auto ident = std::get<IdentToken>(std::get<Token>(qr.block.value[i].value)).value;
                        auto value = std::string{};
                        const auto& value_token = std::get<Token>(qr.block.value[i + 2].value);

                        std::visit(Kori::VariantOverloadSet {
                            [&](const HashToken& token) { value = token.value; },
                            [&](const NumberToken& token) { value = std::to_string(token.value); },
                            [&](const DimensionToken& token) { value = std::format("{}{}", token.value, token.unit); },
                            [](auto&&) { HANAMI_NOT_IMPLEMENTED(); }
                        }, value_token);

                        style_declaration.add_property(ident, value);
                    }

                    rule_objects.emplace_back(style_rule);
                }
            }, rule);
        }

        std::println("Parsed Rules:");
        for (const auto* rule : rule_objects)
        {
            std::println("\t{}", rule->type_str());

            switch (rule->type())
            {
                case Rule::Type::StyleRule:
                {
                    const auto* style_rule = dynamic_cast<const StyleRule*>(rule);
                    std::println("\t\tSelector: {}", style_rule->selector_text());

                    std::println("\t\tProperties:");
                    for (const auto&[name, value] : style_rule->style_declaration().properties())
                    {
                        std::println("\t\t\t{}: {}", name, value);
                    }

                    break;
                }
                default:
                    HANAMI_NOT_IMPLEMENTED();
            }
        }
    }

    auto Parser::consume_next_input_token() -> const Token&
    {
        if (m_next_input_token >= m_tokens.size())
        {
            // TODO(Peter): This is an error since we didn't receive (or at least process)
            //              an EOF-token before this.
            HANAMI_TRAP();
        }

        return m_tokens[m_next_input_token++];
    }

    auto Parser::current_input_token() -> const Token&
    {
        return m_tokens[m_next_input_token - 1];
    }

    auto Parser::consume_list_of_rules(bool) -> std::vector<RuleVariant>
    {
        // Create an initially empty list of rules.
        auto rules = std::vector<RuleVariant>{};

        while (true)
        {
            // Repeatedly consume the next input token:
            const auto& token = consume_next_input_token();

            // <whitespace-token>
            if (std::holds_alternative<WhitespaceToken>(token))
            {
                // Do nothing.
                continue;
            }

            // <EOF-token>
            if (std::holds_alternative<EOFToken>(token))
            {
                // Return the list of rules.
                return rules;
            }

            // <CDO-token>
            // <CDC-token>
            if (std::holds_alternative<CDOToken>(token) || std::holds_alternative<CDCToken>(token))
            {
                // If the top-level flag is set, do nothing.
                // Otherwise, reconsume the current input token. Consume a qualified rule. If anything is returned, append it to the list of rules.
                HANAMI_NOT_IMPLEMENTED();
            }

            // <at-keyword-token>
            if (std::holds_alternative<AtKeywordToken>(token))
            {
                // Reconsume the current input token. Consume an at-rule, and append the returned value to the list of rules.
                HANAMI_NOT_IMPLEMENTED();
            }

            // anything else
            // Reconsume the current input token.
            --m_next_input_token;

            // Consume a qualified rule. If anything is returned, append it to the list of rules.
            auto rule = consume_qualified_rule();

            if (rule.has_value())
            {
                rules.emplace_back(rule.value());
            }
        }
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-component-value
    auto Parser::consume_component_value() -> ComponentValue
    {
        // Consume the next input token.
        const auto& token = consume_next_input_token();

        // If the current input token is a <{-token>, <[-token>, or <(-token>
        if (std::holds_alternative<OpenCurlyBracketToken>(token) || std::holds_alternative<OpenSquareBracketToken>(token) || std::holds_alternative<OpenParenthesesToken>(token))
        {
            // consume a simple block and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // Otherwise, if the current input token is a <function-token>
        if (std::holds_alternative<FunctionToken>(token))
        {
            // consume a function and return it.
            HANAMI_NOT_IMPLEMENTED();
        }

        // Otherwise, return the current input token.
        return { token };
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-qualified-rule
    auto Parser::consume_qualified_rule() -> std::optional<QualifiedRule>
    {
        // Create a new qualified rule with its prelude initially set to an empty list, and its value initially set to nothing.
        auto rule = QualifiedRule{};

        while (true)
        {
            // Repeatedly consume the next input token:
            const auto& token = consume_next_input_token();

            // <EOF-token>
            if (std::holds_alternative<EOFToken>(token))
            {
                // This is a parse error. Return nothing.
                return {};
            }

            // <{-token>
            if (std::holds_alternative<OpenCurlyBracketToken>(token))
            {
                // Consume a simple block and assign it to the qualified rule’s block.
                rule.block = consume_simple_block();

                // Return the qualified rule.
                return rule;
            }

            // TODO(Peter):
                // simple block with an associated token of <{-token>
                // Assign the block to the qualified rule’s block. Return the qualified rule.

            // anything else
            // Reconsume the current input token.
            --m_next_input_token;

            // Consume a component value.
            auto value = consume_component_value();

            // NOTE(Peter): Skip whitespace, not really needed.
            if (const auto* t = std::get_if<Token>(&value.value))
            {
                if (std::holds_alternative<WhitespaceToken>(*t))
                {
                    continue;
                }
            }

            // Append the returned value to the qualified rule’s prelude.
            rule.prelude.emplace_back(value);
        }

        return rule;
    }

    static auto is_ending_token(const SimpleBlockToken& current, const Token& token) -> bool
    {
        bool result = false;

        std::visit(Kori::VariantOverloadSet {
            [&](const OpenSquareBracketToken&) { result = std::holds_alternative<CloseSquareBracketToken>(token); },
            [&](const OpenParenthesesToken&)   { result = std::holds_alternative<CloseParenthesesToken>(token);   },
            [&](const OpenCurlyBracketToken&)  { result = std::holds_alternative<CloseCurlyBracketToken>(token);  }
        }, current.value);

        return result;
    }

    // https://www.w3.org/TR/css-syntax-3/#consume-a-simple-block
    auto Parser::consume_simple_block() -> SimpleBlock
    {
        // Create a simple block with its associated token set to the current input token and with its value initially set to an empty list.
        auto block = SimpleBlock { current_input_token(), {} };

        while (true)
        {
            // Repeatedly consume the next input token and process it as follows:
            const auto& token = consume_next_input_token();

            // ending token
            if (is_ending_token(block.token, token))
            {
                // Return the block.
                return block;
            }

            // <EOF-token>
            if (std::holds_alternative<EOFToken>(token))
            {
                // This is a parse error. Return the block.
                return block;
            }

            // anything else
            // Reconsume the current input token.
            --m_next_input_token;

            // Consume a component value and append it to the value of the block.
            auto value = consume_component_value();

            // NOTE(Peter): Skip whitespace, not really needed.
            if (const auto* t = std::get_if<Token>(&value.value))
            {
                if (std::holds_alternative<WhitespaceToken>(*t))
                {
                    continue;
                }
            }

            block.value.emplace_back(value);
        }
    }

}
