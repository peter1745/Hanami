#pragma once

#include "WebEngine/Core/Core.hpp"

#include <ranges>

namespace Hanami::CSS {

    // https://www.w3.org/TR/cssom/#the-cssrule-interface
    class Rule
    {
    public:
        virtual ~Rule() = default;

        enum class Type : uint8_t
        {
            StyleRule       = 1,
            CharsetRule     = 2,
            ImportRule      = 3,
            MediaRule       = 4,
            FontFaceRule    = 5,
            PageRule        = 6,
            MarginRule      = 9,
            NamespaceRule   = 10
        };

        [[nodiscard]]
        auto type() const noexcept -> Type { return m_type; }

        [[nodiscard]]
        auto type_str() const noexcept -> std::string_view
        {
            switch (m_type)
            {
                case Type::StyleRule: return "StyleRule";
                case Type::CharsetRule: return "CharsetRule";
                case Type::ImportRule: return "ImportRule";
                case Type::MediaRule: return "MediaRule";
                case Type::FontFaceRule: return "FontFaceRule";
                case Type::PageRule: return "PageRule";
                case Type::MarginRule: return "MarginRule";
                case Type::NamespaceRule: return "NamespaceRule";
                default: HANAMI_TRAP();
            }

            return "";
        }

    protected:
        Rule(Type type)
            : m_type(type) {}

    private:
        Type m_type;

        std::string m_text;
        // Rule* m_parent_rule = nullptr;
        // StyleSheet* m_parent_style_sheet = nullptr;
    };

    struct SimpleSelector
    {
        std::string tag;
        std::optional<std::string> id{};
        std::vector<std::string> classes{};
        std::unordered_map<std::string, std::string> attributes{};
        std::vector<std::string> psuedo_classes{};
    };

    enum class Combinator
    {
        None,
        Descendant,
        Child,
        AdjacentSibling,
        GeneralSibling
    };

    struct Selector
    {
        std::vector<std::pair<Combinator, SimpleSelector>> parts;

        [[nodiscard]]
        auto to_string() const noexcept -> std::string
        {
            auto result = std::string{};

            for (const auto& part : parts | std::views::values)
            {
                result += part.tag;

                if (part.id.has_value())
                {
                    result += std::format("#{}", part.id.value());
                }

                for (const auto& class_name : part.classes)
                {
                    result += std::format(".{}", class_name);
                }

                for (const auto& [name, value] : part.attributes)
                {
                    result += std::format("[{}=\"{}\"]", name, value);
                }
            }

            return result;
        }
    };

    class StyleDeclaration
    {
    public:
        using PropertyList = std::vector<std::pair<std::string, std::string>>;

        void add_property(std::string_view name, std::string_view value)
        {
            m_properties.emplace_back(name, value);
        }

        [[nodiscard]]
        auto properties() const noexcept -> const PropertyList&
        {
            return m_properties;
        }

    private:
        PropertyList m_properties;
    };

    // https://www.w3.org/TR/cssom/#the-cssstylerule-interface
    class StyleRule : public Rule
    {
    public:
        StyleRule(Selector selector);

        [[nodiscard]]
        auto style_declaration() noexcept -> StyleDeclaration&
        {
            return m_declaration;
        }

        [[nodiscard]]
        auto style_declaration() const noexcept -> const StyleDeclaration&
        {
            return m_declaration;
        }

        [[nodiscard]]
        auto selector() const noexcept -> const Selector&
        {
            return m_selector;
        }

        [[nodiscard]]
        auto selector_text() const noexcept -> std::string_view
        {
            return m_selector_text;
        }

    private:
        Selector m_selector;
        StyleDeclaration m_declaration;

        std::string m_selector_text;
    };

    struct StyleSheet
    {
        std::vector<std::unique_ptr<Rule>> rules;
    };

}
