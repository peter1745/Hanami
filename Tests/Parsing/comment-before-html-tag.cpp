#include "WebEngine/HTML/Parser.hpp"

#include "../Test.hpp"

DEFINE_SIMPLE_HTML_TEST("Tests/Parsing/comment-before-html-tag.html",
{
    if (doc->children().size() < 2)
    {
        // Failed to load, or didn't parse correctly
        TEST_FAIL("Not enough child nodes");
    }

    const auto* comment = doc->children()[1];

    if (!comment)
    {
        TEST_FAIL("Invalid Comment node");
    }

    if (comment->type() != Hanami::DOM::NodeType::Comment)
    {
        TEST_FAIL("Not a Comment node");
    }

    TEST_PASS();
});
