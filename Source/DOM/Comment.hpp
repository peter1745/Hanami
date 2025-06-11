#pragma once

#include "CharacterData.hpp"

namespace hanami::dom {

    // https://dom.spec.whatwg.org/#interface-comment
    class Comment : public CharacterData
    {
    public:
        Comment(std::string_view data = "") noexcept
            : CharacterData(NodeType::Comment, data)
        {}
    };

}
