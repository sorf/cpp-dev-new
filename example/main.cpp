#include "dev_new.hpp"

#include <iostream>
#include <memory>
#include <string>

template <typename T> void f(T const &v) { std::cout << "f: " << v << std::endl; }

void print_allocations(std::size_t line) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::printf("allocations @line: %zu total: %zu live: %zu\n", line, dev_new::total_allocations(),
                dev_new::live_allocations());
}

#define PRINT_ALLOCATIONSAT_LINE() print_allocations(__LINE__)

int main() {
    {
        PRINT_ALLOCATIONSAT_LINE();
        std::string s{"s"};
        PRINT_ALLOCATIONSAT_LINE();
        auto i = std::make_unique<int>(0);
        PRINT_ALLOCATIONSAT_LINE();
        f(s);
        f(*i);
        PRINT_ALLOCATIONSAT_LINE();
    }
    PRINT_ALLOCATIONSAT_LINE();
    return 0;
}
