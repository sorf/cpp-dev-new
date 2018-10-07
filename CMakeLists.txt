cmake_minimum_required(VERSION 3.1)

project(
    dev_new
    VERSION 0.1
    DESCRIPTION "C++ memory allocator for development and testing"
    LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(dev_new STATIC source/include/dev_new.hpp source/lib/dev_new.cpp)
target_include_directories(dev_new PUBLIC source/include)
target_compile_features(dev_new PUBLIC cxx_std_17)
target_link_libraries(dev_new CONAN_PKG::boost)
set_target_properties(dev_new PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CLANG_TIDY_COMMAND)
    set_target_properties(dev_new PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()
