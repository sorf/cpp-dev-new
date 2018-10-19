#include "dev_new.hpp"

#include <iostream>
#include <string>

void test_function() {
    std::string s0;
    std::string s1("a");
    std::string s2("bc");
    std::string s30("012345678901234567890123456789");
    auto sum = s0 + s1 + s2 + s30;
    DEV_NEW_ASSERT(sum.length() == 33);
}

int main() {
    std::uint64_t error_countdown = 1;
    bool retry = true;
    while (retry) {
        dev_new::pause_error_testing();
        std::cout << "\n#### Error countdown: " << error_countdown << " ####" << std::endl;
        dev_new::set_error_countdown(error_countdown);
        try {
            test_function();
        } catch (std::exception &e) {
            dev_new::pause_error_testing();
            std::cout << "Run error: " << e.what() << std::endl;
        }
        retry = 0 == dev_new::get_error_countdown();
        ++error_countdown;
    }

    return 0;
}
