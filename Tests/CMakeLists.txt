cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE HANAMI_TEST_FILES "*.cpp")

foreach(TEST_MAIN IN LISTS HANAMI_TEST_FILES)
    cmake_path(GET TEST_MAIN FILENAME TEST_NAME)
    string(REPLACE ".cpp" "" TEST_NAME ${TEST_NAME})
    string(PREPEND TEST_NAME "test-")
    add_executable(${TEST_NAME} ${TEST_MAIN})
    target_link_libraries(${TEST_NAME} PRIVATE hanami-webengine kori)
endforeach()

file(CREATE_LINK ${CMAKE_SOURCE_DIR}/Tests ${CMAKE_CURRENT_BINARY_DIR}/Tests SYMBOLIC)

message("Registered Tests: ${HANAMI_TEST_FILES}")
