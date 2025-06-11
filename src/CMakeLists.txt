cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(cairo REQUIRED cairo)

add_executable(hanami)

target_sources(hanami
    PRIVATE
        main.cpp

        # HTML
        html/tokenizer.cpp
        html/tree_builder.cpp
        html/parser.cpp)

target_include_directories(hanami
    PRIVATE
        ../src/)

target_link_libraries(hanami
        PRIVATE
            mwl
            kori
            cairo)
