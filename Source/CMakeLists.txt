cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(cairo REQUIRED cairo)
pkg_check_modules(simdjson REQUIRED simdjson)

add_executable(hanami)

target_sources(hanami
    PRIVATE
        Main.cpp

        # DOM
        DOM/Node.cpp

        # HTML
        HTML/Tokenizer.cpp
        HTML/Parser.cpp)

target_include_directories(hanami
    PRIVATE
        ./)

target_link_libraries(hanami
        PRIVATE
            mwl
            kori
            cairo
            simdjson)

if (HANAMI_PLATFORM_LINUX)
    target_compile_definitions(hanami PUBLIC HANAMI_PLATFORM_LINUX)
elseif(HANAMI_PLATFORM_WINDOWS)
    target_compile_definitions(hanami PUBLIC HANAMI_PLATFORM_WINDOWS)
endif()

# Download the official list of named character references
file(DOWNLOAD https://html.spec.whatwg.org/entities.json ${CMAKE_SOURCE_DIR}/NamedCharacterReferences.json)
