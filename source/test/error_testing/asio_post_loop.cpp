// Error testing of a loop (chain) of asio::io_context::post() calls.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace asio = boost::asio;

namespace {

void posted_func(asio::io_context &io_context, unsigned count) {
    dev_new::run_no_error_testing([&] { std::cout << "posted_func: " << count << std::endl; });
    if (count != 0) {
        asio::post(io_context, [&io_context, count] { posted_func(io_context, count - 1); });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        asio::post(io_context, [&io_context, count = 3] { posted_func(io_context, count); });
        io_context.run();
    });
    return 0;
}
