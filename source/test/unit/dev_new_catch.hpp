#ifndef DEV_NEW_CATCH_HPP
#define DEV_NEW_CATCH_HPP

#include "dev_new.hpp"
#include <boost/scope_exit.hpp>
#include <catch2/catch.hpp>

#define DEV_NEW_END_TEST() dev_new::pause_error_testing();

#define DEV_NEW_REQUIRE(expression)                                                                                    \
    ::dev_new::run_no_error_testing([] { REQUIRE(DEV_NEW_RUN_ERROR_TESTING(expression)); })

#define DEV_NEW_CHECK(expression) ::dev_new::run_no_error_testing([] { CHECK(DEV_NEW_RUN_ERROR_TESTING(expression)); })

#define DEV_NEW_CHECK_THROWS_AS(expression, exceptionType)                                                             \
    ::dev_new::run_no_error_testing([] { CHECK_THROWS_AS(DEV_NEW_RUN_ERROR_TESTING(expression), exceptionType); })

#endif
