#include "dev_new.hpp"

#include <cassert>
#include <cstdlib>
#include <limits>
#include <new>
#include <unordered_set>

#include <boost/intrusive/parent_from_member.hpp>
#include <boost/scope_exit.hpp>

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

// Malloc based allocator.
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

// Object created for each allocation.
struct allocation_object {
    explicit allocation_object(std::size_t count) : count{count}, ptr{} {}
    ~allocation_object() = default;

    allocation_object(allocation_object const & /*unused*/) = delete;
    allocation_object(allocation_object && /*unused*/) = delete;
    allocation_object &operator=(allocation_object const & /*unused*/) = delete;
    allocation_object &operator=(allocation_object && /*unused*/) = delete;

    std::size_t count;
    // User data starts here (aligned as size_t).
    std::size_t ptr;
};

// Allocation memory manager.
class memory_manager {
  public:
    static memory_manager &instance();
    static memory_manager *instance(std::nothrow_t const & /*unused*/) noexcept;

    memory_manager(memory_manager const & /*unused*/) = delete;
    memory_manager(memory_manager && /*unused*/) = delete;
    memory_manager &operator=(memory_manager const & /*unused*/) = delete;
    memory_manager &operator=(memory_manager && /*unused*/) = delete;

    std::uint64_t total_allocations() const noexcept { return m_total_allocations; }
    std::uint64_t live_allocations() const noexcept { return m_pointers.size(); }

    std::uint64_t max_allocated_size() const noexcept { return m_max_allocated_size; }
    std::uint64_t allocated_size() const noexcept { return m_allocated_size; }

    void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept;
    void *allocate(std::size_t count);
    void deallocate(void *ptr) noexcept;

  private:
    memory_manager() : m_total_allocations{}, m_allocated_size{}, m_max_allocated_size{} {}
    ~memory_manager() = default;

    using pointer_set = std::unordered_set<void *, std::hash<void *>, std::equal_to<>, mallocator<void *>>;

    pointer_set m_pointers;
    std::uint64_t m_total_allocations;
    std::uint64_t m_allocated_size;
    std::uint64_t m_max_allocated_size;
};

memory_manager &memory_manager::instance() {
    static memory_manager m;
    return m;
}

memory_manager *memory_manager::instance(std::nothrow_t const & /*unused*/) noexcept {
    memory_manager *m = nullptr;
    try {
        m = &instance();
    } catch (std::exception &) {
    }
    return m;
}

void *memory_manager::allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    void *ptr = nullptr;
    try {
        ptr = allocate(count);
    } catch (std::exception &) {
    }
    return ptr;
}

void *memory_manager::allocate(std::size_t count) {
    void *allocation_ptr = malloc_allocate(sizeof(allocation_object) + count);
    bool commit = false;
    BOOST_SCOPE_EXIT_ALL(&) {
        if (!commit) {
            malloc_deallocate(allocation_ptr);
            allocation_ptr = nullptr;
        }
    };
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto allocation = new (allocation_ptr) allocation_object(count);
    BOOST_SCOPE_EXIT_ALL(&) {
        if (!commit) {
            allocation->~allocation_object();
        }
    };

    void *user_ptr = &allocation->ptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
    assert(m_pointers.count(user_ptr) == 0);
    m_pointers.insert(user_ptr);

    commit = true;
    ++m_total_allocations;
    m_allocated_size += count;
    m_max_allocated_size = std::max(m_max_allocated_size, m_allocated_size);
    return user_ptr;
}

void memory_manager::deallocate(void *ptr) noexcept {
    if (ptr != nullptr && m_pointers.erase(ptr) != 0) {
        auto user_ptr = static_cast<std::size_t *>(ptr);
        auto allocation = boost::intrusive::get_parent_from_member(user_ptr, &allocation_object::ptr);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        assert(allocation->count <= m_allocated_size);
        m_allocated_size -= allocation->count;
        void *allocation_ptr = allocation;
        allocation->~allocation_object();
        malloc_deallocate(allocation_ptr);
    }
}

} // namespace detail

std::uint64_t total_allocations() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->total_allocations();
    }
    return 0;
}

std::uint64_t live_allocations() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->live_allocations();
    }
    return 0;
}

std::uint64_t max_allocated_size() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->max_allocated_size();
    }
    return 0;
}

std::uint64_t allocated_size() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->allocated_size();
    }
    return 0;
}

void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->allocate(count, std::nothrow);
    }
    return nullptr;
}

void *allocate(std::size_t count) { return detail::memory_manager::instance().allocate(count); }

void deallocate(void *ptr) noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        m->deallocate(ptr);
    }
}

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
