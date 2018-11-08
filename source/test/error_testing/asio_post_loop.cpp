#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/io_context.hpp>
#include <asio/post.hpp>

namespace {

void posted_func(asio::io_context &io_context, unsigned count) {
    dev_new::run_no_error_testing([&] { std::cout << "posted_func: " << count << std::endl; });
    if (count != 0) {
        io_context.post([&io_context, count] { posted_func(io_context, count - 1); });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        io_context.post([&io_context, count = 3] { posted_func(io_context, count); });
        io_context.run();
    });
    return 0;
}
