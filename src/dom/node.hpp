#pragma once

#include "core/core.hpp"

namespace hanami::html {

    class TreeBuilder;

}

namespace hanami::dom {

    using namespace std::literals;

    // https://infra.spec.whatwg.org/#namespaces
    static constexpr auto html_namespace = "http://www.w3.org/1999/xhtml"sv;
    static constexpr auto math_ml_namespace = "http://www.w3.org/1998/Math/MathML"sv;
    static constexpr auto svg_namespace = "http://www.w3.org/2000/svg"sv;
    static constexpr auto xlink_namespace = "http://www.w3.org/1999/xlink"sv;
    static constexpr auto xml_namespace = "http://www.w3.org/XML/1998/namespace"sv;
    static constexpr auto xmlns_namespace = "http://www.w3.org/2000/xmlns/"sv;

    enum class NodeType : uint8_t
    {
        Invalid = 0,
        Element = 1,
        Attribute = 2,
        Text = 3,
        CDATASection = 4,
        EntityReference = 5, // legacy
        Entity = 6, // legacy
        ProcessingInstruction = 7,
        Comment = 8,
        Document = 9,
        DocumentType = 10,
        DocumentFragment = 11,
        Notation = 12, // legacy
    };

    inline auto node_type_str(NodeType type) -> std::string_view
    {
        switch (type)
        {
            case NodeType::Invalid: return "Invalid";
            case NodeType::Element: return "Element";
            case NodeType::Attribute: return "Attribute";
            case NodeType::Text: return "Text";
            case NodeType::CDATASection: return "CDATASection";
            case NodeType::EntityReference: return "EntityReference";
            case NodeType::Entity: return "Entity";
            case NodeType::ProcessingInstruction: return "ProcessingInstruction";
            case NodeType::Comment: return "Comment";
            case NodeType::Document: return "Document";
            case NodeType::DocumentType: return "DocumentType";
            case NodeType::DocumentFragment: return "DocumentFragment";
            case NodeType::Notation: return "Notation";
            default: return "Unknown";
        }
    }

    class Node;
    using NodeList = std::vector<Node*>;

    struct NodeListLocation
    {
        NodeList* list;
        NodeList::iterator iter;

        auto operator--(int) const -> NodeListLocation
        {
            auto copy = *this;

            if (copy.iter != copy.list->begin())
            {
                --copy.iter;
            }

            return copy;
        }

        auto operator*() const -> Node*
        {
            if (iter < list->begin() || iter >= list->end())
            {
                return nullptr;
            }

            return *iter;
        }
    };

    class Document;

    // https://dom.spec.whatwg.org/#node
    class Node // : EventTarget
    {
    public:
        virtual ~Node() noexcept = default;

        [[nodiscard]]
        auto first_child() const noexcept -> Node*;

        [[nodiscard]]
        auto last_child() const noexcept -> Node*;

        auto insert_before(Node* node, Node* child) -> Node*;
        auto append_child(Node* node) -> Node*;

        [[nodiscard]]
        bool is_element() const noexcept { return m_type == NodeType::Element; }

        [[nodiscard]]
        auto children() const noexcept -> const NodeList& { return m_child_nodes; }

    protected:
        Node(NodeType type) noexcept
            : m_type(type)
        {
        }

    private:
        NodeType m_type = NodeType::Invalid;
        Document* m_document = nullptr;

        Node* m_parent = nullptr;
        NodeList m_child_nodes{};

        Node* m_previous_sibling = nullptr;
        Node* m_next_sibling = nullptr;

        friend html::TreeBuilder;
    };

}
