#ifndef HPM_GATEWAY_CONTROL_LATEST_BUFFER_HPP
#define HPM_GATEWAY_CONTROL_LATEST_BUFFER_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace hpm_gateway_control {

template <typename T, std::size_t SlotCount = 3>
class LatestBuffer final {
    static_assert(SlotCount >= 3, "LatestBuffer needs at least three slots");
    static_assert(std::is_trivially_copyable_v<T>,
                  "LatestBuffer is for fixed-size snapshots");

public:
    bool publish(const T & value) noexcept
    {
        const std::size_t write = write_index_.load(std::memory_order_relaxed);
        const std::size_t read = read_index_.load(std::memory_order_acquire);
        const std::size_t next = (write + 1U) % SlotCount;
        if (next == read) {
            return false;
        }

        slots_[next] = value;
        write_index_.store(next, std::memory_order_release);
        return true;
    }

    bool read(T & value) noexcept
    {
        const std::size_t ready = write_index_.load(std::memory_order_acquire);
        const std::size_t read = read_index_.load(std::memory_order_relaxed);
        if (ready == read) {
            return false;
        }

        value = slots_[ready];
        read_index_.store(ready, std::memory_order_release);
        return true;
    }

private:
    std::array<T, SlotCount> slots_{};
    std::atomic<std::size_t> write_index_{0U};
    std::atomic<std::size_t> read_index_{0U};
};

}  // namespace hpm_gateway_control

#endif
