#pragma once

#include "Node.hpp"

namespace Hanami::HTML {
    class Parser;
}

namespace Hanami::DOM {

    // https://dom.spec.whatwg.org/#characterdata
    class CharacterData : public Node
    {
    public:
        auto data() const noexcept -> std::string_view
        {
            return m_data;
        }

    protected:
        CharacterData(NodeType type, std::string_view data) noexcept
            : Node(type), m_data(data) {}
    protected:
        std::string m_data;

        friend HTML::Parser;
    };

}
