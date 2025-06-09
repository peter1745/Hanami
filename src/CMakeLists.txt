cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(hanami)

target_sources(hanami
    PRIVATE
        main.cpp

        # HTML
        html/tokenizer.cpp
        html/tree_builder.cpp
        html/parser.cpp)

target_link_libraries(hanami
        PRIVATE
            mwl
            kori)
