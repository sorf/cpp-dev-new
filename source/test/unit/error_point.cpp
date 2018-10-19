#include "dev_new.hpp"
#include "dev_new_catch.hpp"

TEST_CASE("basic test", "[error_point]") {
    dev_new::set_error_countdown(3);
    DEV_NEW_REQUIRE(dev_new::get_error_countdown() == 3);
    dev_new::error_point();
    dev_new::error_point();
    DEV_NEW_CHECK_THROWS_AS(dev_new::error_point(), std::bad_alloc);
    DEV_NEW_CHECK(dev_new::get_error_countdown() == 0);
    DEV_NEW_END_TEST();
}
