#include "zk/core/main_thread_queue.hpp"

#include "zk/log/logger.hpp"

namespace zk::core {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

MainThreadQueue& MainThreadQueue::instance() noexcept
{
    static MainThreadQueue s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// post — callable from any thread
// ---------------------------------------------------------------------------

void MainThreadQueue::post(std::function<void()> fn)
{
    if (!fn) return;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(fn));
    }

    ZK_LOG(zk::log::Level::Trace, zk::log::Domain::Core,
           "MainThreadQueue: callable posted");
}

// ---------------------------------------------------------------------------
// flush — must be called from Main_Thread
// ---------------------------------------------------------------------------

void MainThreadQueue::flush() noexcept
{
    // Swap the live queue into a local one under the lock, then release the
    // lock before executing any callable.  This prevents deadlocks when a
    // callable itself calls post(), and keeps the critical section minimal.
    std::queue<std::function<void()>> local;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local.swap(queue_);
    }

    if (local.empty()) return;

    ZK_LOG(zk::log::Level::Trace, zk::log::Domain::Core,
           "MainThreadQueue: flushing pending callables");

    while (!local.empty()) {
        auto& fn = local.front();
        try {
            fn();
        } catch (...) {
            // Swallow exceptions to keep the message loop alive (Req 9.6).
            ZK_LOG(zk::log::Level::Error, zk::log::Domain::Core,
                   "MainThreadQueue: callable threw an exception (ignored)");
        }
        local.pop();
    }
}

} // namespace zk::core
