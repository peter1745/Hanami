#include "WebEngine/HTML/Parser.hpp"

#include "../Test.hpp"
#include "WebEngine/DOM/HTMLElement.hpp"

DEFINE_SIMPLE_HTML_TEST("Tests/Parsing/eof-when-parsing-last-char.html",
{
    if (doc->children().size() != 2 || !doc->head())
    {
        HTML_TEST_FAIL("Parse failure");
    }

    const auto* html_elem = dynamic_cast<Hanami::DOM::HTMLHtmlElement*>(doc->children()[1]);

    if (!html_elem)
    {
        HTML_TEST_FAIL("Incorrect HTML structure");
    }

    if (html_elem->local_name != "html")
    {
        HTML_TEST_FAIL("No html tag?");
    }

    HTML_TEST_PASS();
});
