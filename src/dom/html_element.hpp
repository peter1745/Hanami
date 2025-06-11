#pragma once

#include "element.hpp"

namespace hanami::dom {

    // https://html.spec.whatwg.org/multipage/dom.html#htmlelement
    class HTMLElement : public Element
    {
    public:
        HTMLElement() noexcept = default;
    };

    // https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
    class HTMLHtmlElement : public HTMLElement
    {
    public:
        HTMLHtmlElement() noexcept
            : HTMLElement() {}
    };

}
