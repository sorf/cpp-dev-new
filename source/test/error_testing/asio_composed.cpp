// Error testing of the Asio composed operation example.
// See: https://github.com/chriskohlhoff/asio/blob/master/asio/src/examples/cpp11/operations/composed_5.cpp
// for detailed comments.
#include "dev_new.hpp"
#include "run_loop.hpp"

//
// composed_5.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_future.hpp>
#include <asio/write.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

using asio::ip::tcp;

// Composed operation.
template <typename T, typename CompletionToken>
auto async_write_messages(tcp::socket &socket, T const &message, std::size_t repeat_count, CompletionToken &&token)
    // The return type of the initiating function is deduced from the CompletionToken type and the completion handler.
    -> typename asio::async_result<typename std::decay<CompletionToken>::type, void(std::error_code)>::return_type {

    using completion_handler_sig = void(std::error_code);
    using completion_type = asio::async_completion<CompletionToken, completion_handler_sig>;
    using completion_handler_type = typename completion_type::completion_handler_type;

    // The intermediate completion handler.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
    struct intermediate_completion_handler {

        tcp::socket &m_socket;
        std::unique_ptr<std::string> m_encoded_message;
        std::size_t m_repeat_count;
        std::unique_ptr<asio::steady_timer> m_delay_timer;
        enum { starting, waiting, writing } m_state;
        asio::executor_work_guard<tcp::socket::executor_type> m_io_work;
        completion_handler_type m_user_handler;

        void operator()(const std::error_code &error, std::size_t /*unused*/ = 0) {
            if (!error) {
                switch (m_state) {
                case starting:
                case writing:
                    if (m_repeat_count > 0) {
                        --m_repeat_count;
                        m_state = waiting;
                        m_delay_timer->expires_after(std::chrono::seconds(1));
                        m_delay_timer->async_wait(std::move(*this));
                        return; // Composed operation not yet complete.
                    }
                    break; // Composed operation complete, continue below.
                case waiting:
                    m_state = writing;
                    asio::async_write(m_socket, asio::buffer(*m_encoded_message), std::move(*this));
                    return; // Composed operation not yet complete.
                }
            }

            // We get here only on completion of the entire composed operation.
            m_io_work.reset();         // no future work for the I/O executor.
            m_encoded_message.reset(); // deallocate the encoded message
            m_user_handler(error);     // Call the user-supplied handler with the result of the operation.
        }

        // For correctness we must preserve the executor of the user-supplied completion handler.
        using executor_type = asio::associated_executor_t<completion_handler_type, tcp::socket::executor_type>;
        executor_type get_executor() const noexcept {
            return asio::get_associated_executor(m_user_handler, m_socket.get_executor());
        }

        // Although not necessary for correctness, we preserve the allocator of the user-supplied completion handler.
        using allocator_type = asio::associated_allocator_t<completion_handler_type, std::allocator<void>>;
        allocator_type get_allocator() const noexcept {
            return asio::get_associated_allocator(m_user_handler, std::allocator<void>{});
        }
    };

    completion_type completion(token);

    std::ostringstream os;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
    os << message;
    std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

    std::unique_ptr<asio::steady_timer> delay_timer(new asio::steady_timer(socket.get_executor().context()));

    // Initiate the underlying operations by calling the intermediate completion handler's function call operator.
    intermediate_completion_handler{socket,
                                    std::move(encoded_message),
                                    repeat_count,
                                    std::move(delay_timer),
                                    intermediate_completion_handler::starting,
                                    asio::make_work_guard(socket.get_executor()),
                                    std::move(completion.completion_handler)}({});

    return completion.result.get();
}

// Test our asynchronous operation using a lambda as a callback.
void test_callback() {
    asio::io_context io_context;
#ifndef __clang_analyzer__
    tcp::acceptor acceptor(io_context, {tcp::v4(), 55555});
    tcp::socket socket = acceptor.accept();
#else
    tcp::socket socket(io_context);
#endif
    async_write_messages(socket, "Testing callback\r\n", 5, [](const std::error_code &error) {
        if (!error) {
            std::cout << "Messages sent\n";
        } else {
            std::cout << "Error: " << error.message() << "\n";
        }
    });

    io_context.run();
}

// Test our asynchronous operation using the use_future completion token.
void test_future() {
    asio::io_context io_context;
#ifndef __clang_analyzer__
    tcp::acceptor acceptor(io_context, {tcp::v4(), 55555});
    tcp::socket socket = acceptor.accept();
#else
    tcp::socket socket(io_context);
#endif
    std::future<void> f = async_write_messages(socket, "Testing future\r\n", 5, asio::use_future);
    io_context.run();

    try {
        // Get the result of the operation.
        f.get();
        std::cout << "Messages sent\n";
    } catch (std::exception const &e) {
        std::cout << "Error: " << e.what() << "\n";
    }

}

int main() {
    try {
        test_callback();
        test_future();
    } catch (std::exception const &e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}
