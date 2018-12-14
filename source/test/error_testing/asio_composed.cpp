// Error testing of the Asio composed operation example.
// Based on https://github.com/chriskohlhoff/asio/blob/master/asio/src/examples/cpp11/operations/composed_5.cpp
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/bind_executor.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_future.hpp>
#include <boost/format.hpp>
#include <boost/range/irange.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>

template <typename Executor, typename CompletionToken>
auto async_many_timers(Executor &&executor, std::chrono::steady_clock::duration run_duration, CompletionToken &&token)
    -> typename asio::async_result<std::decay_t<CompletionToken>, void(std::error_code)>::return_type {

    using completion_handler_sig = void(std::error_code);
    using completion_type = asio::async_completion<CompletionToken, completion_handler_sig>;
    using completion_handler_type = typename completion_type::completion_handler_type;

    struct internal_state : public std::enable_shared_from_this<internal_state> {
        internal_state(Executor &&executor_, completion_handler_type user_completion_handler_)
            : executor(std::move(executor_)),
              user_completion_handler{std::move(user_completion_handler_)}, run_timer{executor.context()},
              internal_timers(25), is_open(false) {
            for (auto &timer : internal_timers) {
                timer.reset(new asio::steady_timer{executor.context()});
            }
        }

        void start_many_waits(std::chrono::steady_clock::duration run_duration) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> one_wait_distribution(1, internal_timers.size());

            // Setting the total run duration.
            run_timer.expires_after(run_duration);
            run_timer.async_wait(
                asio::bind_executor(get_executor(), [this, self = this->shared_from_this()](asio::error_code ec) {
                    close();
                    // TODO: This should be called after all the internal timers have completed their cancelling
                    // (no just initiated)
                    user_completion_handler(ec);
                }));

            // Starting the internal timers in parallel.
            for (auto timer_index : boost::irange<std::size_t>(0, internal_timers.size())) {
                start_one_wait(timer_index, std::chrono::milliseconds(one_wait_distribution(gen)));
            }
            dev_new::run_no_error_testing([&] { std::cout << "timers started" << std::endl; });
            is_open = true;
        }

        void close() {
            run_timer.cancel();
            for (auto &timer : internal_timers) {
                timer->cancel();
            }
            dev_new::run_no_error_testing([&] { std::cout << "timers cancelled" << std::endl; });
        }

        void start_one_wait(std::size_t timer_index, std::chrono::steady_clock::duration one_wait) {
            auto &timer = internal_timers[timer_index];
            timer->expires_after(one_wait);
            timer->async_wait(
                asio::bind_executor(get_executor(), [this, timer_index, one_wait = std::move(one_wait),
                                                     self = this->shared_from_this()](asio::error_code ec) {
                    if (is_open) {
                        start_one_wait(timer_index, std::move(one_wait));
                    } else if (!is_open) {
                        dev_new::run_no_error_testing(
                            [&] { std::cout << boost::format("timer[%d]: wait closed") % timer_index << std::endl; });
                    } else if (ec == asio::error::operation_aborted) {
                        dev_new::run_no_error_testing([&] {
                            std::cout << boost::format("timer[%d]: wait cancelled") % timer_index << std::endl;
                        });
                    } else {
                        dev_new::run_no_error_testing([&] {
                            std::cout << boost::format("timer[%d]: wait error: %s:%s") % ec % ec.message() << std::endl;
                        });
                        close();
                    }
                }));
        }

        using executor_type = asio::associated_executor_t<completion_handler_type, Executor>;
        executor_type get_executor() const noexcept {
            return asio::get_associated_executor(user_completion_handler, executor);
        }

        Executor executor;
        completion_handler_type user_completion_handler;
        asio::steady_timer run_timer;
        std::vector<std::unique_ptr<asio::steady_timer>> internal_timers;
        bool is_open;
    };

    completion_type completion(token);
    std::make_shared<internal_state>(std::move(executor), std::move(completion.completion_handler))
        ->start_many_waits(run_duration);
    return completion.result.get();
}

void test_callback(std::chrono::steady_clock::duration run_duration) {
    asio::io_context io_context;
    async_many_timers(io_context.get_executor(), run_duration, [](std::error_code const &error) {
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
    std::future<void> f = async_many_timers(io_context.get_executor(), run_duration, asio::use_future);
    io_context.run();

    try {
        f.get();
        std::cout << "[future] Timers done" << std::endl;
    } catch (std::exception const &e) {
        std::cout << "[future] Timers error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        test_callback(std::chrono::seconds(3));
        test_future(std::chrono::seconds(3));
    } catch (std::exception const &e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}
