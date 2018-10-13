#include "dev_new.hpp"

#include <cstdlib>
#include <limits>
#include <mutex>
#include <new>
#include <unordered_set>

#include <boost/intrusive/parent_from_member.hpp>
#include <boost/scope_exit.hpp>

namespace dev_new {

void assertion_failed(char const *expr, char const *function, char const *file, std::size_t line) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::fprintf(stderr, "dev_new assertion failed: %s (function: %s, file: %s, line: %zu)\n", expr, function, file,
                 line);
    std::abort();
}

void assertion_failed_msg(char const *expr, char const *msg, char const *function, char const *file, std::size_t line) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::fprintf(stderr, "dev_new assertion failed: %s) (message: %s, function: %s, file: %s, line: %zu)\n", expr, msg,
                 function, file, line);
    std::abort();
}

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
    static auto const magic_value = 0x0123ABCD6789CDEFULL;
    explicit allocation_object(std::size_t count) : magic{magic_value}, count{count}, ptr{} {}
    ~allocation_object() {
        DEV_NEW_ASSERT(magic == magic_value);
        magic = 0xABCD0123CDEF6789ULL;
    }

    allocation_object(allocation_object const & /*unused*/) = delete;
    allocation_object(allocation_object && /*unused*/) = delete;
    allocation_object &operator=(allocation_object const & /*unused*/) = delete;
    allocation_object &operator=(allocation_object && /*unused*/) = delete;

    std::uint64_t magic;
    std::size_t count;
    // User data starts here (aligned as size_t).
    std::size_t ptr;
};

// Allocation memory manager.
class memory_manager {
  public:
    static memory_manager &instance() {
        static memory_manager m;
        return m;
    }

    static memory_manager *instance(std::nothrow_t const & /*unused*/) noexcept {
        memory_manager *m = nullptr;
        try {
            m = &instance();
        } catch (std::exception &) {
        }
        return m;
    }

    memory_manager(memory_manager const & /*unused*/) = delete;
    memory_manager(memory_manager && /*unused*/) = delete;
    memory_manager &operator=(memory_manager const & /*unused*/) = delete;
    memory_manager &operator=(memory_manager && /*unused*/) = delete;

    std::uint64_t total_allocations() const noexcept {
        lock_guard lock(m_mutex);
        return m_total_allocations;
    }
    std::uint64_t live_allocations() const noexcept {
        lock_guard lock(m_mutex);
        return m_pointers.size();
    }

    std::uint64_t max_allocated_size() const noexcept {
        lock_guard lock(m_mutex);
        return m_max_allocated_size;
    }
    std::uint64_t allocated_size() const noexcept {
        lock_guard lock(m_mutex);
        return m_allocated_size;
    }

    void set_error_countdown(std::uint64_t countdown) noexcept {
        lock_guard lock(m_mutex);
        m_error_testing = true;
        m_error_countdown = countdown;
        // If countdown is zero, all the subsequent allocations will fail.
        m_error_allocated_size = m_allocated_size;
    }

    std::uint64_t get_error_countdown() noexcept {
        lock_guard lock(m_mutex);
        return m_error_countdown;
    }

    void pause_error_testing() noexcept {
        lock_guard lock(m_mutex);
        m_error_testing = false;
    }

    void resume_error_testing() noexcept {
        lock_guard lock(m_mutex);
        m_error_testing = true;
    }

    void error_point() {
        lock_guard lock(m_mutex);
        error_point_implementation(1);
    }

    void *allocate(std::size_t count, std::nothrow_t const & /*unused*/) noexcept {
        void *ptr = nullptr;
        try {
            ptr = allocate(count);
        } catch (std::exception &) {
        }
        return ptr;
    }

    void *allocate(std::size_t count) {
        lock_guard lock(m_mutex);
        error_point_implementation(count);

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
        DEV_NEW_ASSERT(m_pointers.count(user_ptr) == 0);
        m_pointers.insert(user_ptr);

        commit = true;
        ++m_total_allocations;
        m_allocated_size += count;
        m_max_allocated_size = std::max(m_max_allocated_size, m_allocated_size);
        return user_ptr;
    }

    void deallocate(void *ptr) noexcept {
        lock_guard lock(m_mutex);
        if (ptr != nullptr && m_pointers.erase(ptr) != 0) {
            auto user_ptr = static_cast<std::size_t *>(ptr);
            auto allocation = boost::intrusive::get_parent_from_member(user_ptr, &allocation_object::ptr);
            DEV_NEW_ASSERT(allocation->count <= m_allocated_size);
            m_allocated_size -= allocation->count;
            void *allocation_ptr = allocation;
            allocation->~allocation_object();
            malloc_deallocate(allocation_ptr);
        }
    }

    void check_allocation(void *ptr) {
        lock_guard lock(m_mutex);
        if (m_pointers.count(ptr) == 0) {
            throw std::domain_error("dev_new: pointer not allocated by this allocator");
        }
    }

    bool check_allocation(void *ptr, std::nothrow_t const & /*unused*/) noexcept {
        lock_guard lock(m_mutex);
        return m_pointers.count(ptr) != 0;
    }

  private:
    memory_manager()
        : m_total_allocations{}, m_allocated_size{}, m_max_allocated_size{}, m_error_testing{},
          m_error_countdown{UINT64_MAX}, m_error_allocated_size{UINT64_MAX} {}
    ~memory_manager() = default;

    void error_point_implementation(std::size_t count) {
        if (m_error_testing) {
            if (m_error_countdown > 1) {
                --m_error_countdown;
            } else if (m_error_countdown == 1) {
                m_error_allocated_size = m_allocated_size;
                m_error_countdown = 0;
                throw std::bad_alloc();
            } else if (m_allocated_size + count > m_error_allocated_size) {
                throw std::bad_alloc();
            }
        }
    }

    using pointer_set = std::unordered_set<void *, std::hash<void *>, std::equal_to<>, mallocator<void *>>;
    using lock_guard = std::lock_guard<std::mutex>;

    mutable std::mutex m_mutex;
    pointer_set m_pointers;
    std::uint64_t m_total_allocations;
    std::uint64_t m_allocated_size;
    std::uint64_t m_max_allocated_size;

    bool m_error_testing;
    std::uint64_t m_error_countdown;
    std::uint64_t m_error_allocated_size;
};

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

void set_error_countdown(std::uint64_t countdown) noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        m->set_error_countdown(countdown);
    }
}

std::uint64_t get_error_countdown() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->get_error_countdown();
    }
    return 0;
}

void pause_error_testing() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        m->pause_error_testing();
    }
}

void resume_error_testing() noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        m->resume_error_testing();
    }
}

void error_point() { detail::memory_manager::instance().error_point(); }

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

void check_allocation(void *ptr) {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        m->check_allocation(ptr);
    }
}

bool check_allocation(void *ptr, std::nothrow_t const & /*unused*/) noexcept {
    if (auto m = detail::memory_manager::instance(std::nothrow)) {
        return m->check_allocation(ptr, std::nothrow);
    }
    return false;
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
