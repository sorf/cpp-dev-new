// Error testing of a loop (chain) of asio::steady_timer::async_wait() calls with memory moving at each call.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <vector>

namespace asio = boost::asio;

namespace {

using error_code = boost::system::error_code;
using buffer_t = std::vector<char>;

void do_wait(asio::steady_timer &timer, unsigned count, buffer_t buffer) {
    dev_new::run_no_error_testing([&] { std::cout << "do_wait: " << count << std::endl; });
    if (count != 0) {
        timer.expires_after(std::chrono::milliseconds(10));
        timer.async_wait([&timer, count, buffer = std::move(buffer)](error_code ec) mutable {
            if (!ec) {
                do_wait(timer, count - 1, std::move(buffer));
            } else {
                dev_new::run_no_error_testing([&] { std::cout << "do_wait error: " << ec << std::endl; });
            }
        });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        asio::steady_timer timer(io_context);
        do_wait(timer, 3, buffer_t(10));
        io_context.run();
    });
    return 0;
}
