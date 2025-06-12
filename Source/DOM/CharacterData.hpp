#pragma once

#include "Node.hpp"

namespace Hanami::HTML {
    class Parser;
}

namespace Hanami::DOM {

    // https://dom.spec.whatwg.org/#characterdata
    class CharacterData : public Node
    {
    protected:
        CharacterData(NodeType type, std::string_view data) noexcept
            : Node(type), m_data(data) {}
    protected:
        std::string m_data;

        friend HTML::Parser;
    };

}
