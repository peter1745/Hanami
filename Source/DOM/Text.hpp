#pragma once

#include "CharacterData.hpp"

namespace Hanami::DOM {

    // https://dom.spec.whatwg.org/#interface-text
    class Text : public CharacterData
    {
    public:
        Text(std::string_view data = "")
            : CharacterData(NodeType::Text, data) {}

        [[nodiscard]]
        auto whole_text() const noexcept -> std::string_view
        {
            return m_data;
        }
    };

}
