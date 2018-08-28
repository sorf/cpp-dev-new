#ifndef DEV_NEW_HPP
#define DEV_NEW_HPP

#include <cstdlib>
#include <new>

namespace dev_new {

std::size_t total_allocations();
std::size_t live_allocations();

void *allocate(std::size_t count, std::nothrow_t const & /*unused*/);
void *allocate(std::size_t count);
void deallocate(void *ptr);

} // namespace dev_new

#endif
