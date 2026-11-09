#pragma once
#include "Trajectory3D.hpp"
#include <atomic>

namespace orbital {
// A value snapshot of the *effective*, smoothed DSP controls, never a UI pointer.
struct VisualizationState {
    Path path;
    std::array<double, count> coefficients{};
    double phase = 0., spin = 0., frequency = 0.;
};

// Single audio producer / single UI consumer. Ownership of each slot is transferred by
// release/acquire indices. Unlike a seqlock, non-atomic payloads are never read while written.
// If the UI stops, audio drops snapshots; neither side waits or allocates.
class VisualizationQueue {
    static constexpr unsigned capacity = 8;
    std::array<VisualizationState, capacity> slots{};
    std::atomic<unsigned> write{0}, read{0};

  public:
    VisualizationQueue() {
        static_assert(ATOMIC_INT_LOCK_FREE == 2, "Visualization requires lock-free indices");
    }
    bool publish(const VisualizationState &s) noexcept {
        const auto w = write.load(std::memory_order_relaxed);
        const auto next = (w + 1) % capacity;
        if (next == read.load(std::memory_order_acquire))
            return false;
        slots[w] = s;
        write.store(next, std::memory_order_release);
        return true;
    }
    bool latest(VisualizationState &s) noexcept {
        auto r = read.load(std::memory_order_relaxed);
        // Bound the work even if audio keeps publishing while this function runs.
        const auto end = write.load(std::memory_order_acquire);
        if (r == end)
            return false;
        while (r != end) {
            s = slots[r];
            r = (r + 1) % capacity;
        }
        read.store(r, std::memory_order_release);
        return true;
    }
};
} // namespace orbital
