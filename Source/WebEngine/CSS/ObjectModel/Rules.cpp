#include "Rules.hpp"

#include <ranges>

namespace Hanami::CSS {

    StyleRule::StyleRule(Selector selector)
            : Rule(Type::StyleRule), m_selector(std::move(selector)), m_selector_text(m_selector.to_string())
    {
    }

}

