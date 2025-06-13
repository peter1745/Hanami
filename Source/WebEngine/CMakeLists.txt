cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(cairo REQUIRED cairo)
pkg_check_modules(simdjson REQUIRED simdjson)

add_library(hanami-webengine)

target_sources(hanami-webengine
    PRIVATE
        # DOM
        DOM/Node.cpp

        # HTML
        HTML/Tokenizer.cpp
        HTML/Parser.cpp)

target_include_directories(hanami-webengine PUBLIC ../)

target_link_libraries(hanami-webengine
    PRIVATE
        kori
        simdjson)

if (HANAMI_PLATFORM_LINUX)
    target_compile_definitions(hanami-webengine PUBLIC HANAMI_PLATFORM_LINUX)
elseif(HANAMI_PLATFORM_WINDOWS)
    target_compile_definitions(hanami-webengine PUBLIC HANAMI_PLATFORM_WINDOWS)
endif()
