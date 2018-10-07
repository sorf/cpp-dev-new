cmake_minimum_required(VERSION 3.1)

project(
    dev_new_example
    LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(example example.cpp)
target_compile_features(example PUBLIC cxx_std_17)
target_link_libraries(example CONAN_PKG::dev_new)
if(CLANG_TIDY_COMMAND)
    set_target_properties(example PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()
