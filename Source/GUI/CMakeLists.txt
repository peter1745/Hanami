cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig)
pkg_check_modules(cairo REQUIRED cairo)

add_executable(hanami-gui)

target_sources(hanami-gui PRIVATE Main.cpp)
target_link_libraries(hanami-gui
    PRIVATE
        mwl
        kori
        cairo
        hanami-webengine)

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/Tests ${CMAKE_CURRENT_BINARY_DIR}/Tests SYMBOLIC)
