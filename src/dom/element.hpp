#pragma once

#include "node.hpp"

namespace hanami::dom {

    // https://dom.spec.whatwg.org/#interface-element
    class Element : public Node
    {
    public:
        Element() noexcept
            : Node(NodeType::Element) {}

        // Null or a non-empty string.
        std::optional<std::string_view> namespace_uri{std::nullopt};

        // Null or a non-empty string.
        std::optional<std::string_view> namespace_prefix{std::nullopt};

        // A non-empty string.
        std::string local_name;

        // TODO(Peter): Custom Elements
        // custom element registry
        // Null or a CustomElementRegistry object.
        // custom element state
        // "undefined", "failed", "uncustomized", "precustomized", or "custom".
        // custom element definition
        // Null or a custom element definition.
        // is value
        // Null or a valid custom element name.

        [[nodiscard]]
        auto is_in_namespace(std::string_view value) const noexcept -> bool
        {
            return namespace_uri == value;
        }
    };

}
