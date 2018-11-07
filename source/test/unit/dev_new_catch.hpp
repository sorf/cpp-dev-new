#ifndef DEV_NEW_CATCH_HPP
#define DEV_NEW_CATCH_HPP

#include "dev_new.hpp"
#include <boost/scope_exit.hpp>
#include <catch2/catch.hpp>

#define DEV_NEW_END_TEST() dev_new::pause_error_testing();

template <typename F> decltype(auto) dev_new_wrap(F const &f) {
    dev_new::resume_error_testing();
    BOOST_SCOPE_EXIT_ALL(&) { dev_new::pause_error_testing(); };
    return f();
}

#define DEV_NEW_WRAP(expression) dev_new_wrap([] { return expression; })

#define DEV_NEW_REQUIRE(expression)                                                                                    \
    do {                                                                                                               \
        dev_new::pause_error_testing();                                                                                \
        REQUIRE(DEV_NEW_WRAP(expression));                                                                             \
        dev_new::resume_error_testing();                                                                               \
    } while (false)

#define DEV_NEW_CHECK(expression)                                                                                      \
    do {                                                                                                               \
        dev_new::pause_error_testing();                                                                                \
        CHECK(DEV_NEW_WRAP(expression));                                                                               \
        dev_new::resume_error_testing();                                                                               \
    } while (false)

#define DEV_NEW_CHECK_THROWS_AS(expression, exceptionType)                                                             \
    do {                                                                                                               \
        dev_new::pause_error_testing();                                                                                \
        CHECK_THROWS_AS(DEV_NEW_WRAP(expression), exceptionType);                                                      \
        dev_new::resume_error_testing();                                                                               \
    } while (false)

#endif
