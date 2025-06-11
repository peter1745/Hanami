#include "Node.hpp"
#include "Element.hpp"
#include "Document.hpp"

namespace Hanami::DOM {

    auto Node::first_child() const noexcept -> Node*
    {
        if (m_child_nodes.empty())
        {
            return nullptr;
        }

        return m_child_nodes.front();
    }

    auto Node::last_child() const noexcept -> Node*
    {
        if (m_child_nodes.empty())
        {
            return nullptr;
        }

        return m_child_nodes.back();
    }

    // https://dom.spec.whatwg.org/#concept-node-pre-insert
    auto Node::insert_before(Node* node, Node* child) -> Node*
    {
        // 1. TODO: Ensure pre-insert validity of node into parent before child.
        // 2. Let referenceChild be child.
        auto* reference_child = child;

        // 3. If referenceChild is node, then set referenceChild to node’s next sibling.
        if (reference_child == node)
        {
            reference_child = node->m_next_sibling;
        }

        // 4. Insert node into parent before referenceChild.
        auto reference_child_pos = std::ranges::find(m_child_nodes, reference_child);
        m_child_nodes.insert(reference_child_pos, node);

        // FIXME(Peter): Set Document m_body
        if (auto* elem = dynamic_cast<Element*>(node); elem && elem->local_name == "body")
        {
            m_document->m_body = elem;
        }

        // FIXME(Peter): Hack around using the proper insertion steps.
        node->m_document = m_type == NodeType::Document ? dynamic_cast<Document*>(this) : m_document;
        node->m_parent = this;

        // 5. Return node.
        return node;
    }

    // https://dom.spec.whatwg.org/#concept-node-append
    auto Node::append_child(Node* node) -> Node*
    {
        return insert_before(node, nullptr);
    }

}
