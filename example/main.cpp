#include "dev_new.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

template <typename T> void f(T const &v) { std::cout << "f: " << v << std::endl; }

void print_allocations(std::size_t line) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::printf("allocations @line: %zu allocations: total: %zu live: %zu; allocated: max: %zu now: %zu\n", line,
                dev_new::total_allocations(), dev_new::live_allocations(), dev_new::max_allocated_size(),
                dev_new::allocated_size());
}

#define PRINT_ALLOCATIONS_AT_LINE() print_allocations(__LINE__)

int main() {
    {
        PRINT_ALLOCATIONS_AT_LINE();
        auto s = std::make_unique<std::string>("s");
        PRINT_ALLOCATIONS_AT_LINE();
        auto i = std::make_unique<int>(0);
        PRINT_ALLOCATIONS_AT_LINE();
        [[maybe_unused]] std::vector<char> v(128);
        f(*s);
        f(*i);
        PRINT_ALLOCATIONS_AT_LINE();
    }
    PRINT_ALLOCATIONS_AT_LINE();
    return 0;
}
