#include "dev_new.hpp"

void basic_error_point() {
    dev_new::set_error_countdown(3);
    dev_new::error_point();
    dev_new::error_point();
    try {
        dev_new::error_point();
        DEV_NEW_ASSERT(false);
    } catch (std::bad_alloc &) {

    } catch (...) {
        DEV_NEW_ASSERT(false);
    }
}

int main() {
    basic_error_point();
    return 0;
}
