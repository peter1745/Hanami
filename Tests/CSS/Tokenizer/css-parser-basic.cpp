#include "../../Test.hpp"

#include <WebEngine/CSS/Parser.hpp>

DEFINE_SIMPLE_TEST({

    Hanami::CSS::Parser::parse_from_file("Tests/CSS/Tokenizer/css-tokenizer-basic.css");

    TEST_PASS();
});
