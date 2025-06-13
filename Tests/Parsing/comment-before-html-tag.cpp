#include "WebEngine/HTML/Parser.hpp"

int main()
{
    Hanami::HTML::Parser{}.parse("Tests/Parsing/comment-before-html-tag.html");
    return 0;
}
