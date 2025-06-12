#pragma once

#include "Node.hpp"
#include "Element.hpp"

namespace Hanami::HTML {

    class Parser;

}

namespace Hanami::DOM {

    // https://dom.spec.whatwg.org/#interface-documenttype
    class DocumentType : public Node
    {
    public:
        DocumentType(std::string_view name, std::string_view public_id, std::string_view system_id) noexcept
            : Node(NodeType::DocumentType), m_name(name), m_public_id(public_id), m_system_id(system_id)
        {
        }

        [[nodiscard]]
        auto name() const noexcept -> std::string_view { return m_name; }

        [[nodiscard]]
        auto public_id() const noexcept -> std::string_view { return m_public_id; }

        [[nodiscard]]
        auto system_id() const noexcept -> std::string_view { return m_system_id; }

    private:
        std::string m_name;
        std::string m_public_id;
        std::string m_system_id;
    };

    // https://html.spec.whatwg.org/multipage/dom.html#document
    class Document : public Node
    {
    public:
        Document() noexcept
            : Node(NodeType::Document)
        {}

        [[nodiscard]]
        auto head() const noexcept -> Element* { return m_head; }

        [[nodiscard]]
        auto body() const noexcept -> Element* { return m_body; }

    private:
        Element* m_head = nullptr;
        Element* m_body = nullptr;
        bool m_scripting = false;

        friend HTML::Parser;
        friend class Node;
    };


}
