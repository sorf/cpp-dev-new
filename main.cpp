#include <iostream>
#include <memory>
#include <string>

#include "dev_new.hpp"

template <typename T> void f(T const &v) { std::cout << "f: " << v << std::endl; }

void printAllocations(std::size_t line) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::printf("allocations @line: %lu total: %lu live: %lu\n", line, dev_new::total_allocations(),
                dev_new::live_allocations());
}

#define PRINT_ALLOCATIONS() printAllocations(__LINE__)

int main() {
    {
        PRINT_ALLOCATIONS();
        std::string s{"s"};
        PRINT_ALLOCATIONS();
        auto i = std::make_unique<int>(0);
        PRINT_ALLOCATIONS();
        f(s);
        f(*i);
        PRINT_ALLOCATIONS();
    }
    PRINT_ALLOCATIONS();
    return 0;
}
