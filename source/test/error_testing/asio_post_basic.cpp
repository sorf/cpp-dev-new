// Basic asio::io_context::post() error testing.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace asio = boost::asio;

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        unsigned s = 0;
        asio::post(io_context, [&] { DEV_NEW_ASSERT(s == 1); });
        s = 1;
        io_context.run();
        s = 2;
    });
    return 0;
}
