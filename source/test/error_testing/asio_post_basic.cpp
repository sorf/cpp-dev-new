#include "dev_new.hpp"
#include "run_loop.hpp"

#include <asio/io_context.hpp>
#include <asio/post.hpp>

int main() {
    error_testing::run_loop([] {
        asio::io_context io_context;
        unsigned s = 0;
        io_context.post([&] { DEV_NEW_ASSERT(s == 1); });
        s = 1;
        io_context.run();
        s = 2;
    });
    return 0;
}
