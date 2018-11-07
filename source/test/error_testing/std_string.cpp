#include "dev_new.hpp"
#include "run_loop.hpp"
#include <string>

int main() {
    error_testing::run_loop([] {
        std::string s0;
        std::string s1("a");
        std::string s2("bc");
        std::string s30("012345678901234567890123456789");
        auto sum = s0 + s1 + s2 + s30;
        DEV_NEW_ASSERT(sum.length() == 33);
    });
    return 0;
}
