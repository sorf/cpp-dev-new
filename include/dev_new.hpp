#ifndef DEV_NEW_HPP
#define DEV_NEW_HPP

#include <cstdint>
#include <cstdlib>
#include <new>

namespace dev_new {

std::uint64_t total_allocations() noexcept;
std::uint64_t live_allocations() noexcept;
std::uint64_t max_allocated_size() noexcept;
std::uint64_t allocated_size() noexcept;

void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept;
void *allocate(std::size_t count);
void deallocate(void *ptr) noexcept;

} // namespace dev_new

#endif
