cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(TestRunner TestRunner.cpp)

add_compile_definitions(TESTS_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE HANAMI_TEST_FILES "*.cpp")

foreach(TEST_MAIN IN LISTS HANAMI_TEST_FILES)
    string(FIND ${TEST_MAIN} "TestRunner" TEST_RUNNER_FOUND)

    if (TEST_RUNNER_FOUND EQUAL -1)
        message("Registered test: ${TEST_MAIN}")
        cmake_path(GET TEST_MAIN FILENAME TEST_NAME)
        string(REPLACE ".cpp" "" TEST_NAME ${TEST_NAME})
        string(PREPEND TEST_NAME "test-")
        add_executable(${TEST_NAME} ${TEST_MAIN})
        target_link_libraries(${TEST_NAME} PRIVATE hanami-webengine kori)

        add_dependencies(TestRunner ${TEST_NAME})
    endif()

endforeach()

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/Tests ${CMAKE_CURRENT_BINARY_DIR}/Tests SYMBOLIC)
