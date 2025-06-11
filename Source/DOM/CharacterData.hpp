#pragma once

#include "Node.hpp"

namespace hanami::html {
    class TreeBuilder;
}

namespace hanami::dom {

    // https://dom.spec.whatwg.org/#characterdata
    class CharacterData : public Node
    {
    protected:
        CharacterData(NodeType type, std::string_view data) noexcept
            : Node(type), m_data(data) {}
    protected:
        std::string m_data;

        friend html::TreeBuilder;
    };

}
