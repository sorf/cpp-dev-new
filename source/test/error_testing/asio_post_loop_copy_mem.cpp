// Error testing of a loop (chain) of asio::io_context::post() calls with memory copying at each call.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <vector>

namespace {

using buffer_t = std::vector<char>;

// NOLINTNEXTLINE(performance-unnecessary-value-param)
void posted_func(asio::io_context &io_context, unsigned count, buffer_t buffer) {
    dev_new::run_no_error_testing([&] { std::cout << "posted_func: " << count << std::endl; });
    if (count != 0) {
        asio::post(io_context, [&io_context, count, buffer] { posted_func(io_context, count - 1, buffer); });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        asio::post(io_context,
                   [&io_context, count = 3, buffer = buffer_t(10)] { posted_func(io_context, count, buffer); });
        io_context.run();
    });
    return 0;
}
