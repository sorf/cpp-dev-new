#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <vector>

namespace {

using buffer = std::vector<char>;

// NOLINTNEXTLINE(performance-unnecessary-value-param)
void posted_func(asio::io_context &io_context, unsigned count, buffer buf) {
    dev_new::run_no_error_testing([&] { std::cout << "posted_func: " << count << std::endl; });
    if (count != 0) {
        io_context.post([&io_context, count, buf] { posted_func(io_context, count - 1, buf); });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        io_context.post([&io_context, count = 3, buf = buffer(10)] { posted_func(io_context, count, buf); });
        io_context.run();
    });
    return 0;
}
