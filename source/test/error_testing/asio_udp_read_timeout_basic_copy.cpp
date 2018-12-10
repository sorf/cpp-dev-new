// Read from a UDP socket with timeout.
#include "dev_new.hpp"
#include "run_loop.hpp"

#include <array>
#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <vector>

namespace {

using buffer_t = std::vector<char>;

} // namespace

int main() {
    using asio::ip::udp;
    error_testing::run_loop([] {
        asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), 5678));
        std::array<char, 1024> data = {};
        udp::endpoint sender_endpoint;
        asio::steady_timer timeout_timer(io_context);
        buffer_t timeout_buffer(10);

        // Wait to receive something
        socket.async_receive_from(
            asio::buffer(data), sender_endpoint, [](asio::error_code ec, std::size_t bytes_recvd) {
                if (!ec) {
                    dev_new::run_no_error_testing([&] { std::cout << "received: " << bytes_recvd << std::endl; });
                } else if (ec == asio::error::operation_aborted) {
                    dev_new::run_no_error_testing([&] { std::cout << "received: cancelled" << std::endl; });
                } else {
                    dev_new::run_no_error_testing([&] { std::cout << "received error: " << ec << std::endl; });
                }
            });

        // Timeout
        timeout_timer.expires_after(std::chrono::milliseconds(100));
        timeout_timer.async_wait([&, timeout_buffer](asio::error_code /*unused*/) {
#if 0 // Execution blocks if we enable this

            // Simulate a handler copy operation before being called.
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            [[maybe_unused]] auto const timeout_buffer_copy = timeout_buffer;

#endif
            asio::error_code ignored;
            socket.close(ignored);
        });

        while (!io_context.stopped()) {
            try {
                io_context.run();
            } catch (std::exception &e) {
                dev_new::run_no_error_testing([&] { std::cout << "io_context run error: " << e.what() << std::endl; });
            }
        }
    });
    return 0;
}
