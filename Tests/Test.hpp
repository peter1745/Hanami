#pragma once

#include <print>

#define TEST_FAIL(msg) status = -1; return
#define TEST_PASS() status = 0; return

#define DEFINE_SIMPLE_HTML_TEST(file, test)\
    int main()\
    {\
        const auto* doc = Hanami::HTML::Parser::parse_from_file(file);\
        if (!doc) { return -1; }\
        int status = -1;\
        [&] test();\
        delete doc;\
        return status;\
    }

#define DEFINE_SIMPLE_TEST(test)\
    int main()\
    {\
        int status = -1;\
        [&] test();\
        return status;\
    }

    