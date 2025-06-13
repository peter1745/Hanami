#include "WebEngine/HTML/Parser.hpp"

int main()
{
    Hanami::HTML::Parser{}.parse("Tests/Parsing/eof-when-parsing-last-char.html");
    return 0;
}
