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
        Main.cpp

        # DOM
        DOM/Node.cpp

        # HTML
        HTML/Tokenizer.cpp
        HTML/TreeBuilder.cpp
        HTML/Parser.cpp)

target_include_directories(hanami
    PRIVATE
        ./)

target_link_libraries(hanami
        PRIVATE
            mwl
            kori
            cairo)
