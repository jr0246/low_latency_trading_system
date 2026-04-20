#pragma once

#include <atomic>
#include <cassert>

#include "macros.h"

/**
 * SPSC queue
 */
namespace Common {
  template <typename T, typename Alloc = std::allocator<T>>
  class LFQueue : Alloc {
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = allocator_traits::size_type;

  public:
    explicit LFQueue(size_type N, Alloc const& alloc = Alloc{}) :
      Alloc{alloc},
      mask_{N - 1},
      ring_{allocator_traits::allocate(*this, N)} {
      ASSERT(std::has_single_bit(N), "N must be a power of two.");
    }

    template <typename U>
            requires std::is_nothrow_constructible_v<value_type, U&&>
    bool push(U&& elem) {
      auto wr_index = write_index_.load(std::memory_order_relaxed);
      if (full(wr_index, cached_read_index_)) {
        cached_read_index_ = read_index_.load(std::memory_order_acquire);
        if (full(wr_index, cached_read_index_)) {
          return false;
        }
      }
      std::construct_at(getElement(wr_index), std::forward<U>(elem));
      write_index_.store(write_index_ + 1, std::memory_order_release);
      return true;
    }

    bool pop(T& value) {
      auto rd_index = read_index_.load(std::memory_order_relaxed);
      if (empty(cached_write_index_, rd_index)) {
        cached_write_index_ = write_index_.load(std::memory_order_acquire);
        if (empty(cached_write_index_, rd_index)) {
          return false;
        }
      }
      value = *getElement(rd_index);
      getElement(rd_index)->~T();
      read_index_.store(read_index_ + 1, std::memory_order_release);
      return true;
    }

    auto size() const noexcept {
      auto pushCursor = write_index_.load(std::memory_order_relaxed);
      auto popCursor = read_index_.load(std::memory_order_relaxed);

      assert(popCursor <= pushCursor);
      return pushCursor - popCursor;
    }

    LFQueue(const LFQueue&) = delete;
    LFQueue& operator=(const LFQueue&) = delete;

    LFQueue(LFQueue&&) = delete;
    LFQueue& operator=(LFQueue&&) = delete;

  private:
    size_type mask_;
    T* ring_;

    static constexpr auto hardware_destructive_interference_size = size_type{64};

    /** Atomic read/write indices. Aligned on unique cache lines to avoid false sharing between the producer and
     * consumer threads. We include a cached version of each to allow lazy index updates, which is valid in an
     * SPSC context. The cached versions do not need to be atomics as each is only ever read by one of the
     * producer (cached read) or consumer (cached write).
     */
    alignas(hardware_destructive_interference_size) std::atomic<size_type> write_index_ = {0};
    alignas(hardware_destructive_interference_size) size_type cached_write_index_ = {0};

    alignas(hardware_destructive_interference_size) std::atomic<size_type> read_index_ = {0};
    alignas(hardware_destructive_interference_size) size_type cached_read_index_ = {0};

    auto capacity() const noexcept {
      return mask_ + 1;
    }

    auto full(size_type pushCursor, size_type popCursor) const noexcept {
      return pushCursor - popCursor == capacity();
    }

    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
      return pushCursor == popCursor;
    }

    auto getElement(size_type cursor) {
      return &ring_[cursor & mask_];
    }
  };
}
