#ifndef DEV_NEW_HPP
#define DEV_NEW_HPP

#include <boost/scope_exit.hpp>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace dev_new {

void assertion_failed(char const *expr, char const *function, char const *file, std::size_t line);
void assertion_failed_msg(char const *expr, char const *msg, char const *function, char const *file, std::size_t line);

std::uint64_t total_allocations() noexcept;
std::uint64_t live_allocations() noexcept;
std::uint64_t max_allocated_size() noexcept;
std::uint64_t allocated_size() noexcept;

/// Sets the number of allocations and/or error points until an out-of-memory condition will be simulated.
/// Once the simulated out-of-memory condition is reached, subsequent allocations will fail if they would lead to
/// allocating more memory than what was allocated when the condition was reached.
void set_error_countdown(std::uint64_t countdown) noexcept;
std::uint64_t get_error_countdown() noexcept;

/// Pause and resume raising errors when allocating memory.
/// This is useful for disabling memory errors while logging, reporting an error or calling a unit-test framework.
// \{
void pause_error_testing() noexcept;
void resume_error_testing() noexcept;
// \}

/// Defines an error point.
/// This call behaves as if allocating memory some memory and failing if we are in the simulated out-of-memory
/// condition.
void error_point();

/// Raw allocation and deallocation.
// \{
void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept;
void *allocate(std::size_t count);
void deallocate(void *ptr) noexcept;
// \}

/// Checks that a pointer has been allocated by this allocator.
/// Throws an error if the test fails.
void check_allocation(void *ptr);
/// Checks that a pointer has been allocated by this allocator.
bool check_allocation(void *ptr, std::nothrow_t const & /*unused*/) noexcept;

/// Runs a function under resume/pause error testing.
template <typename F> decltype(auto) run_error_testing(F const &f) {
    resume_error_testing();
    BOOST_SCOPE_EXIT_ALL(&) { pause_error_testing(); };
    return f();
}

/// Runs a function under pause/resume error testing.
template <typename F> decltype(auto) run_no_error_testing(F const &f) {
    pause_error_testing();
    BOOST_SCOPE_EXIT_ALL(&) { resume_error_testing(); };
    return f();
}

} // namespace dev_new

/// Assertion macros.
/// They are defined all the time, regardless of build type.
// \{
#define DEV_NEW_ASSERT(expr)                                                                                           \
    ((expr) ? ((void)0) : ::dev_new::assertion_failed(#expr, static_cast<char const *>(__func__), __FILE__, __LINE__))

#define DEV_NEW_ASSERT_MSG(expr, msg)                                                                                  \
    ((expr) ? ((void)0)                                                                                                \
            : ::dev_new::assertion_failed_msg(#expr, msg, static_cast<char const *>(__func__), __FILE__, __LINE__))
// \}

/// Runs an expression under resume/pause error testing.
#define DEV_NEW_RUN_ERROR_TESTING(expression) ::dev_new::run_error_testing([&] { return expression; })

/// Runs an expression under pause/resume error testing.
#define DEV_NEW_RUN_NO_ERROR_TESTING(expression) ::dev_new::run_no_error_testing([&] { return expression; })

#endif
