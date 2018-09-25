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
target_include_directories(dev_new SYSTEM PRIVATE $ENV{BOOST_INCLUDE_FOLDER} )
target_compile_features(dev_new PUBLIC cxx_std_17)
set_target_properties(dev_new PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CLANG_TIDY_EXE)
    set_target_properties(dev_new PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Defines an executable (test or example) target.
function(define_executable category root_name)
    set(executable_binary ${category}_${root_name})
    add_executable(${executable_binary} ${category}/${root_name}.cpp)
    target_compile_features(${executable_binary} PRIVATE cxx_std_17)
    target_include_directories(${executable_binary} PRIVATE include)
    target_link_libraries(${executable_binary} PRIVATE dev_new)
    if(CLANG_TIDY_EXE)
        set_target_properties(${executable_binary} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
endfunction(define_executable)


# The list of examples.
set(DEV_NEW_EXAMPLES "initial")
foreach(example_name ${DEV_NEW_EXAMPLES})
    define_executable("example" ${example_name})
endforeach(example_name)

# The list of tests.
set(DEV_NEW_TESTS "std_string")
foreach(test_name ${DEV_NEW_TESTS})
    define_executable("test" ${test_name})
endforeach(test_name)
