cmake_minimum_required(VERSION 3.1)

project(
    dev_new
    VERSION 0.1
    DESCRIPTION "C++ memory allocator for development and testing"
    LANGUAGES CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
enable_testing()

add_library(dev_new STATIC source/include/dev_new.hpp source/lib/dev_new.cpp)
target_include_directories(dev_new PUBLIC source/include)
target_compile_features(dev_new PUBLIC cxx_std_17)
target_link_libraries(dev_new CONAN_PKG::boost)
set_target_properties(dev_new PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CLANG_TIDY_COMMAND)
    set_target_properties(dev_new PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

# Defined a test executable
function(define_test_executable category test_name sources)
    set(full_path_sources "")
    foreach(source ${sources})
        message("source: " ${source})
        list(APPEND full_path_sources source/test/${category}/${source})
    endforeach(source)

    set(executable_binary ${category}_${test_name})
    add_executable(${executable_binary} ${full_path_sources})
    target_compile_features(${executable_binary} PRIVATE cxx_std_17)
    target_include_directories(${executable_binary} PRIVATE source/include)
    target_link_libraries(${executable_binary} PRIVATE dev_new)
    target_link_libraries(${executable_binary} PRIVATE CONAN_PKG::boost)
    target_link_libraries(${executable_binary} PRIVATE CONAN_PKG::catch2)
    if(CLANG_TIDY_COMMAND)
        set_target_properties(${executable_binary} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
    add_test(NAME ${executable_binary} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin COMMAND ${executable_binary})
endfunction(define_test_executable)


set(ERROR_TESTING std_string;)
foreach(test_name ${ERROR_TESTING})
    define_test_executable(error_testing ${test_name} ${test_name}.cpp)
endforeach(test_name)

set(UNIT_TESTS error_point.cpp; main.cpp)
define_test_executable(unit tests "${UNIT_TESTS}")
