// Error testing of the Asio composed operation example.
// Based on https://github.com/chriskohlhoff/asio/blob/master/asio/src/examples/cpp11/operations/composed_5.cpp
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/bind_executor.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_future.hpp>
#include <atomic>
#include <boost/format.hpp>
#include <boost/predef.h>
#include <boost/scope_exit.hpp>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <thread>

// A composed operation that is impelmented as a series of timer waits.
template <typename CompletionToken>
auto async_many_timers(asio::io_context &io_context, std::chrono::steady_clock::duration run_duration,
                       CompletionToken &&token) ->
    typename asio::async_result<std::decay_t<CompletionToken>, void(std::error_code)>::return_type {

    using completion_handler_sig = void(std::error_code);
    using completion_type = asio::async_completion<CompletionToken, completion_handler_sig>;
    using completion_handler_type = typename completion_type::completion_handler_type;

    struct internal_state : public std::enable_shared_from_this<internal_state> {
        internal_state(asio::io_context &io_context, completion_handler_type user_completion_handler)
            : io_context{io_context}, user_completion_handler{std::move(user_completion_handler)},
              io_work{asio::make_work_guard(io_context.get_executor())}, run_timer{io_context}, executing{false} {}

        void start_many_waits(std::chrono::steady_clock::duration run_duration) {
            DEV_NEW_ASSERT(!executing);
            executing = true;
            BOOST_SCOPE_EXIT_ALL(&) { executing = false; };

            auto now = std::chrono::steady_clock::now();
            end_run = now + run_duration;
            start_one_wait(now);
            dev_new::run_no_error_testing([&] { std::cout << "timer started" << std::endl; });
        }

        void start_one_wait(std::chrono::steady_clock::time_point now) {
            auto one_wait = std::chrono::milliseconds(5);
            if (end_run < now + one_wait) {
                one_wait = std::chrono::milliseconds(1);
            }

            run_timer.expires_after(one_wait);
            run_timer.async_wait(asio::bind_executor(
                get_executor(), [this, self = this->shared_from_this() ](asio::error_code ec) {
                    DEV_NEW_ASSERT(!executing);
                    executing = true;
                    BOOST_SCOPE_EXIT_ALL(&) { executing = false; };

                    auto now = std::chrono::steady_clock::now();
                    if (!ec && now < end_run) {
                        start_one_wait(now);
                        return;
                    }

                    // The composed operation ended.
                    if (ec) {
                        dev_new::run_no_error_testing(
                            [&] { std::cout << boost::format("timer error: %s:%s") % ec % ec.message() << std::endl; });
                    } else {
                        dev_new::run_no_error_testing([&] { std::cout << "timer complete" << std::endl; });
                    }

                    io_work.reset();
                    user_completion_handler(ec);
                }));
        }

#if BOOST_COMP_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif
        using executor_type = asio::associated_executor_t<completion_handler_type, asio::io_context::executor_type>;
#if BOOST_COMP_CLANG
#pragma clang diagnostic pop
#endif
        executor_type get_executor() const noexcept {
            return asio::get_associated_executor(user_completion_handler, io_context.get_executor());
        }

        asio::io_context &io_context;
        completion_handler_type user_completion_handler;
        asio::executor_work_guard<asio::io_context::executor_type> io_work;

        asio::steady_timer run_timer;
        std::chrono::steady_clock::time_point end_run;
        std::atomic_bool executing;
    };

    completion_type completion(token);
    std::make_shared<internal_state>(io_context, std::move(completion.completion_handler))
        ->start_many_waits(run_duration);
    return completion.result.get();
}

void test_callback(std::chrono::steady_clock::duration run_duration) {
    asio::io_context io_context;
    std::cout << "[callback] Timers start" << std::endl;
    async_many_timers(io_context, run_duration, [](std::error_code const &error) {
        if (error) {
            std::cout << "[callback] Timers error: " << error.message() << std::endl;
        } else {
            std::cout << "[callback] Timers done" << std::endl;
        }
    });

    io_context.run();
}

void test_future(std::chrono::steady_clock::duration run_duration) {
    asio::io_context io_context;
    std::cout << "[future] Timers start" << std::endl;

    std::future<void> f = async_many_timers(io_context, run_duration, asio::use_future);
    std::thread thread_wait_future([f = std::move(f)]() mutable {
        try {
            f.get();
            std::cout << "[future] Timers done" << std::endl;
        } catch (std::exception const &e) {
            std::cout << "[future] Timers error: " << e.what() << std::endl;
        }
    });
    io_context.run();
    thread_wait_future.join();
}

int main() {
    try {
        test_callback(std::chrono::seconds(1));
        test_future(std::chrono::seconds(1));
    } catch (std::exception const &e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}
