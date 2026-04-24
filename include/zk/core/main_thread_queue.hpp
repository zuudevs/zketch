#pragma once

#include <functional>
#include <mutex>
#include <queue>

namespace zk::core {

/// Thread-safe queue for posting callables from any thread to be executed
/// on the Main_Thread.
///
/// ## Usage pattern
///   - Any thread calls `post(callable)` to enqueue work.
///   - The Main_Thread calls `flush()` periodically (every pump iteration)
///     to drain and execute all pending callables in FIFO order.
///
/// ## Thread safety
///   `post()` is safe to call from any thread concurrently.
///   `flush()` must only be called from the Main_Thread (Req 11.1).
///
/// ## Design notes
///   - Uses `std::mutex` + `std::queue` for simplicity and correctness.
///   - The queue is swapped under the lock so that `flush()` releases the
///     mutex before executing callables, preventing deadlocks if a callable
///     itself calls `post()`.
///
/// Satisfies Requirements: 11.4, 18.7
class MainThreadQueue {
public:
    /// Returns the process-wide singleton instance.
    [[nodiscard]] static MainThreadQueue& instance() noexcept;

    // Non-copyable, non-movable singleton.
    MainThreadQueue(const MainThreadQueue&)            = delete;
    MainThreadQueue& operator=(const MainThreadQueue&) = delete;
    MainThreadQueue(MainThreadQueue&&)                 = delete;
    MainThreadQueue& operator=(MainThreadQueue&&)      = delete;

    /// Enqueues a callable to be executed on the Main_Thread.
    ///
    /// Safe to call from any thread.  The callable is copied/moved into the
    /// internal queue and will be invoked during the next `flush()` call.
    ///
    /// @param fn  Callable with signature `void()`.  Must not be null.
    void post(std::function<void()> fn);

    /// Executes all callables currently in the queue, in FIFO order.
    ///
    /// Must be called from the Main_Thread only.  The queue is drained
    /// atomically under the lock before any callable is invoked, so new
    /// items posted during flush will be deferred to the next flush cycle.
    ///
    /// Exceptions thrown by individual callables are caught and silently
    /// discarded to keep the message loop alive (consistent with Req 9.6).
    void flush() noexcept;

private:
    MainThreadQueue() noexcept = default;

    std::mutex                        mutex_;
    std::queue<std::function<void()>> queue_;
};

} // namespace zk::core
