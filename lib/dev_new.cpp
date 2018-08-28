#include "dev_new.hpp"

#include <cassert>
#include <cstdlib>
#include <limits>
#include <new>
#include <unordered_set>

namespace dev_new {

namespace detail {

void *malloc_allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    if (count == 0) {
        return nullptr;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    return std::malloc(count);
}

void *malloc_allocate(std::size_t count) {
    if (count == 0) {
        return nullptr;
    }

    auto *ptr = malloc_allocate(count, std::nothrow);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }

    return ptr;
}

template <typename T> T *malloc_allocate(std::size_t count) {
    if (count == 0) {
        return nullptr;
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
        throw std::bad_array_new_length();
    }
    return static_cast<T *>(malloc_allocate(count * sizeof(T)));
}

void malloc_deallocate(void *ptr) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    std::free(ptr);
}

// Malloc based allocator
// Based on:
// https://stackoverflow.com/a/36521845
template <typename T> struct mallocator {
    using value_type = T;
    mallocator() noexcept = default;
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    template <typename U> mallocator(mallocator<U> const & /*unused*/) noexcept {}
    template <typename U> bool operator==(mallocator<U> const & /*unused*/) const noexcept { return true; }
    template <typename U> bool operator!=(mallocator<U> const & /*unused*/) const noexcept { return false; }

    T *allocate(std::size_t count) const { return malloc_allocate<T>(count); }
    void deallocate(T *const ptr, std::size_t /*unused*/) const noexcept { malloc_deallocate(ptr); }
};

// Allocated memory manager.
class memory_manager {
  public:
    static memory_manager &instance();

    memory_manager(memory_manager const & /*unused*/) = delete;
    memory_manager(memory_manager && /*unused*/) = delete;
    memory_manager &operator=(memory_manager const & /*unused*/) = delete;
    memory_manager &operator=(memory_manager && /*unused*/) = delete;

    std::size_t total_allocations() const;
    std::size_t live_allocations() const;

    void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept;
    void *allocate(std::size_t count);
    void deallocate(void *ptr);

  private:
    memory_manager();
    ~memory_manager();

    using pointer_set = std::unordered_set<void *, std::hash<void *>, std::equal_to<>, mallocator<void *>>;

    pointer_set m_pointers;
    std::size_t m_total_allocations;
};

memory_manager &memory_manager::instance() {
    static memory_manager m;
    return m;
}

memory_manager::memory_manager() : m_total_allocations{} {}

memory_manager::~memory_manager() = default;

std::size_t memory_manager::total_allocations() const { return m_total_allocations; }
std::size_t memory_manager::live_allocations() const { return m_pointers.size(); }

void *memory_manager::allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    void *ptr = nullptr;
    try {
        ptr = allocate(count);
    } catch (std::exception &) {
    }
    return ptr;
}

void *memory_manager::allocate(std::size_t count) {
    void *ptr = malloc_allocate(count);
    if (ptr != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        assert(m_pointers.count(ptr) == 0);
        m_pointers.insert(ptr);
        ++m_total_allocations;
    }
    return ptr;
}

void memory_manager::deallocate(void *ptr) {
    if (ptr != nullptr && m_pointers.erase(ptr) != 0) {
        malloc_deallocate(ptr);
    }
}

} // namespace detail

std::size_t total_allocations() { return detail::memory_manager::instance().total_allocations(); }
std::size_t live_allocations() { return detail::memory_manager::instance().live_allocations(); }

void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) {
    return detail::memory_manager::instance().allocate(count, std::nothrow);
}
void *allocate(std::size_t count) { return detail::memory_manager::instance().allocate(count); }
void deallocate(void *ptr) { detail::memory_manager::instance().deallocate(ptr); }

} // namespace dev_new

void *operator new(std::size_t count) { return dev_new::allocate(count); }
void *operator new[](std::size_t count) { return dev_new::allocate(count); }
void *operator new(std::size_t count, std::align_val_t /*unused*/) { return dev_new::allocate(count); }
void *operator new[](std::size_t count, std::align_val_t /*unused*/) { return dev_new::allocate(count); }
void *operator new(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    return dev_new::allocate(count, std::nothrow);
}
void *operator new[](std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    return dev_new::allocate(count, std::nothrow);
}
void *operator new(std::size_t count, std::align_val_t /*unused*/, std::nothrow_t const & /*unused*/) noexcept {
    return dev_new::allocate(count, std::nothrow);
}
void *operator new[](std::size_t count, std::align_val_t /*unused*/, std::nothrow_t const & /*unused*/) noexcept {
    return dev_new::allocate(count, std::nothrow);
}

void operator delete(void *ptr) noexcept { dev_new::deallocate(ptr); }
void operator delete[](void *ptr) noexcept { dev_new::deallocate(ptr); }
void operator delete(void *ptr, std::align_val_t /*unused*/) noexcept { dev_new::deallocate(ptr); }
void operator delete[](void *ptr, std::align_val_t /*unused*/) noexcept { dev_new::deallocate(ptr); }
void operator delete(void *ptr, std::size_t /*unused*/) noexcept { dev_new::deallocate(ptr); }
void operator delete[](void *ptr, std::size_t /*unused*/) noexcept { dev_new::deallocate(ptr); }
void operator delete(void *ptr, std::size_t /*unused*/, std::align_val_t /*unused*/) noexcept {
    dev_new::deallocate(ptr);
}
void operator delete[](void *ptr, std::size_t /*unused*/, std::align_val_t /*unused*/) noexcept {
    dev_new::deallocate(ptr);
}
