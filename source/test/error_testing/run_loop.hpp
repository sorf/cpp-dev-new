#ifndef ERROR_TESTING_RUN_LOOP_HPP
#define ERROR_TESTING_RUN_LOOP_HPP

#include "dev_new.hpp"
#include <iostream>

namespace error_testing {

template <typename F> void run_loop(F const &f) {
    std::uint64_t error_countdown = 1;
    bool retry = true;
    while (retry) {
        dev_new::pause_error_testing();
        std::cout << "\n#### Error countdown: " << error_countdown << " ####" << std::endl;
        dev_new::set_error_countdown(error_countdown);
        try {
            f();
        } catch (std::exception &e) {
            dev_new::pause_error_testing();
            std::cout << "Run error: " << e.what() << std::endl;
        }
        retry = 0 == dev_new::get_error_countdown();
        ++error_countdown;
    }
}

} // namespace error_testing

#endif
