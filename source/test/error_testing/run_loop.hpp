#ifndef ERROR_TESTING_RUN_LOOP_HPP
#define ERROR_TESTING_RUN_LOOP_HPP

#include "dev_new.hpp"

#include <asio/io_context.hpp>
#include <iostream>

namespace error_testing {

/// Error testing run loop.
/// It calls the given function as long as the error countdown leads to an error being raised.
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
            std::cout << "Run error: " << e.what() << ". Live allocations: " << dev_new::live_allocations()
                      << " Total allocations: " << dev_new::total_allocations() << std::endl;
        }
        retry = 0 == dev_new::get_error_countdown();
        ++error_countdown;
    }
    std::cout << "End execution. Live allocations: " << dev_new::live_allocations()
              << " Total allocations: " << dev_new::total_allocations() << std::endl;
}

/// Runs an io_context as long as it isn't stopped.
/// The execution is resumed if an error is raised from the run() call. As this run() call is assumed to be called
/// from the top level of an application (i.e. directly from main()), there isn't much context we can associate with
/// an exception coming out of it. That's why we simply log the exception and resume the io_context() execution.
/// See also the io_context documentation on the
/// [effect of exceptions thrown from handlers](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/
/// io_context.html#boost_asio.reference.io_context.effect_of_exceptions_thrown_from_handlers).
inline void run_io_context(asio::io_context &io_context) {
    while (!io_context.stopped()) {
        try {
            io_context.run();
        } catch (std::exception &e) {
            dev_new::run_no_error_testing([&] { std::cout << "io_context run error: " << e.what() << std::endl; });
        }
    }
}

} // namespace error_testing

#endif
