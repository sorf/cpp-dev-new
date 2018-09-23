#include "dev_new.hpp"

#include <cinttypes>
#include <iostream>
#include <memory>
#include <vector>

void print_allocations(std::size_t line) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::printf("allocations @line: %zu allocations: total: %" PRIu64 " live %" PRIu64 "; "
                "allocated : max: %" PRIu64 "  now: %" PRIu64 "\n",
                line, dev_new::total_allocations(), dev_new::live_allocations(), dev_new::max_allocated_size(),
                dev_new::allocated_size());
}

#define PRINT_ALLOCATIONS_AT_LINE() print_allocations(__LINE__)

int main() {
    dev_new::set_error_countdown(3);

    {
        PRINT_ALLOCATIONS_AT_LINE();
        [[maybe_unused]] auto i1 = std::make_unique<std::uint16_t>(std::uint16_t(0));
        PRINT_ALLOCATIONS_AT_LINE();
        [[maybe_unused]] auto i2 = std::make_unique<std::uint8_t>(std::uint8_t(0));
        PRINT_ALLOCATIONS_AT_LINE();
        // 3rd allocation (after we allocated 3 bytes)
        try {
            [[maybe_unused]] std::vector<char> v(128);
            std::cerr << "Error: We shouldn't get here !" << std::endl;
        } catch (std::exception &e) {
            dev_new::pause_error_testing();
            std::cout << "Expected error: " << e.what() << std::endl;
            dev_new::resume_error_testing();
        }
        PRINT_ALLOCATIONS_AT_LINE();
    }
    PRINT_ALLOCATIONS_AT_LINE();

    // Re-allocating 3 bytes.
    [[maybe_unused]] auto i1 = std::make_unique<std::uint8_t>(std::uint8_t(0));
    [[maybe_unused]] auto i2 = std::make_unique<std::uint8_t>(std::uint8_t(0));
    [[maybe_unused]] auto i3 = std::make_unique<std::uint8_t>(std::uint8_t(0));
    try {
        // This should fail again as we reach the amount of allocated memory at the time of the first generated error.
        [[maybe_unused]] auto c = std::make_unique<char>('a');
        std::cerr << "Error: We shouldn't get here !" << std::endl;
    } catch (std::exception &e) {
        dev_new::pause_error_testing();
        std::cout << "Expected error: " << e.what() << std::endl;
        dev_new::resume_error_testing();
    }

    return 0;
}
