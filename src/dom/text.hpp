#pragma once

#include "character_data.hpp"

namespace hanami::dom {

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
