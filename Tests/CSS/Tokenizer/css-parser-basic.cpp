#include "../../Test.hpp"

#include <WebEngine/CSS/Parser.hpp>

DEFINE_SIMPLE_TEST({

    Hanami::CSS::Parser::parse_from_file("Tests/CSS/Tokenizer/css-tokenizer-basic.css", {
        .dump_tokens = args.has_arg("--css-dump-tokens") || args.has_arg("--css-dump-all"),
        .dump_rules  = args.has_arg("--css-dump-rules")  || args.has_arg("--css-dump-all")
    });

    TEST_PASS();
}, INCLUDE_ARGS_LIST);
