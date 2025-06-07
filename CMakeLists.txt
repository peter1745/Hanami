cmake_minimum_required(VERSION 3.30)

project(hanami VERSION 0.1.0 LANGUAGES CXX)

if(MSVC)
    string(REGEX REPLACE "/W[1-3]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

if (UNIX AND NOT APPLE)
    set(HANAMI_PLATFORM_LINUX ON)
elseif(WIN32)
    set(HANAMI_PLATFORM_WINDOWS ON)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

add_subdirectory(src)

set(MWL_BUILD_EXAMPLES OFF)
add_subdirectory(mwl)
add_subdirectory(Kori)
