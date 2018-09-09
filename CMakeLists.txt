cmake_minimum_required(VERSION 3.1)

project(
    cpp-dev-new
    VERSION 0.1
    DESCRIPTION "C++ memory allocator for development and testing"
    LANGUAGES CXX)

if (MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        add_compile_options(/W4)
    endif()
    add_compile_options(/WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
    if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
        add_compile_options(-Wglobal-constructors)

        if(CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "6.0")
            message(STATUS "Clang 6.0 detected; trying to enable clang-tidy-6.0...")
            find_program(
                CLANG_TIDY_EXE
                NAMES "clang-tidy-6.0"
                DOC "Path to clang-tidy executable")
            if(NOT CLANG_TIDY_EXE)
                message(FATAL_ERROR "clang-tidy-6.0 not found.")
            else()
                message(STATUS "clang-tidy-6.0 enabled.")
                set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
            endif()
        endif()

    endif()
endif()


add_library(dev_new STATIC include/dev_new.hpp lib/dev_new.cpp)
target_include_directories(dev_new PUBLIC include)
target_compile_features(dev_new PUBLIC cxx_std_17)
set_target_properties(dev_new PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CLANG_TIDY_EXE)
    set_target_properties(dev_new PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

add_executable(example example/main.cpp)
target_include_directories(example PRIVATE include)
target_link_libraries(example PRIVATE dev_new)
if(CLANG_TIDY_EXE)
    set_target_properties(example PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()
