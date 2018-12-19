// Error testing of a loop (chain) of asio::io_context::post() calls with memory moving at each call.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <vector>

namespace asio = boost::asio;

namespace {

using buffer_t = std::vector<char>;

void do_post(asio::io_context &io_context, unsigned count, buffer_t buffer) {
    dev_new::run_no_error_testing([&] { std::cout << "do_post: " << count << std::endl; });
    if (count != 0) {
        asio::post(io_context, [&io_context, count, buffer = std::move(buffer)]() mutable {
            do_post(io_context, count - 1, std::move(buffer));
        });
    }
}

} // namespace

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        do_post(io_context, 3, buffer_t(10));
        io_context.run();
    });
    return 0;
}
