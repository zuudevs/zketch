/// test_main_thread_queue.cpp
///
/// Unit tests for MainThreadQueue.
///
/// Strategy
/// --------
/// MainThreadQueue is a singleton, so each test must drain any residual
/// state left by previous tests.  A helper `drain()` is called at the
/// start of every test case to ensure a clean slate.
///
/// The cross-thread scenario is exercised by spawning a std::thread that
/// calls post(), joining it, then calling flush() on the "main" thread
/// (the test runner thread).  This is sufficient to verify the mutex-
/// protected enqueue path without requiring a real Win32 message loop.
///
/// Requirements covered: 11.4

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "zk/core/main_thread_queue.hpp"

using zk::core::MainThreadQueue;

// ---------------------------------------------------------------------------
// Helper: drain any leftover callables from previous tests
// ---------------------------------------------------------------------------

static void drain()
{
    MainThreadQueue::instance().flush();
}

// ---------------------------------------------------------------------------
// flush on empty queue
// ---------------------------------------------------------------------------

TEST_CASE("MainThreadQueue: flush on empty queue does not error",
          "[core][main_thread_queue]")
{
    // Req 11.4 — flush() must be safe to call when nothing has been posted.
    drain(); // ensure clean state

    REQUIRE_NOTHROW(MainThreadQueue::instance().flush());
    REQUIRE_NOTHROW(MainThreadQueue::instance().flush()); // idempotent
}

// ---------------------------------------------------------------------------
// post + flush on the same thread
// ---------------------------------------------------------------------------

TEST_CASE("MainThreadQueue: callable posted and flushed on same thread is executed",
          "[core][main_thread_queue]")
{
    drain();

    bool called = false;
    MainThreadQueue::instance().post([&called]() { called = true; });

    REQUIRE_FALSE(called); // not yet — flush hasn't run

    MainThreadQueue::instance().flush();

    REQUIRE(called);
}

TEST_CASE("MainThreadQueue: multiple callables are executed in FIFO order",
          "[core][main_thread_queue]")
{
    drain();

    std::vector<int> order;
    MainThreadQueue::instance().post([&order]() { order.push_back(1); });
    MainThreadQueue::instance().post([&order]() { order.push_back(2); });
    MainThreadQueue::instance().post([&order]() { order.push_back(3); });

    MainThreadQueue::instance().flush();

    REQUIRE(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("MainThreadQueue: flush only executes callables posted before the flush call",
          "[core][main_thread_queue]")
{
    // Callables posted *during* flush (from inside a callable) must be
    // deferred to the next flush cycle — the queue is swapped atomically.
    drain();

    int outer_count = 0;
    int inner_count = 0;

    MainThreadQueue::instance().post([&]() {
        ++outer_count;
        // Post a new callable from inside flush — must NOT run this cycle.
        MainThreadQueue::instance().post([&]() { ++inner_count; });
    });

    MainThreadQueue::instance().flush();

    REQUIRE(outer_count == 1);
    REQUIRE(inner_count == 0); // deferred

    // Second flush picks up the inner callable.
    MainThreadQueue::instance().flush();

    REQUIRE(inner_count == 1);
}

// ---------------------------------------------------------------------------
// Cross-thread: post from worker thread, flush from main thread
// ---------------------------------------------------------------------------

TEST_CASE("MainThreadQueue: post from another thread, flush from main thread executes callable",
          "[core][main_thread_queue]")
{
    // Req 11.4 — the primary cross-thread use case.
    drain();

    std::atomic<bool> called{false};

    std::thread worker([&called]() {
        MainThreadQueue::instance().post([&called]() {
            called.store(true, std::memory_order_relaxed);
        });
    });
    worker.join(); // ensure post() has completed before flush()

    REQUIRE_FALSE(called.load()); // not yet

    MainThreadQueue::instance().flush();

    REQUIRE(called.load());
}

TEST_CASE("MainThreadQueue: multiple posts from multiple threads all execute on flush",
          "[core][main_thread_queue]")
{
    // Req 11.4 — concurrent posts from N threads; all callables must run.
    drain();

    constexpr int thread_count = 8;
    std::atomic<int> counter{0};

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        workers.emplace_back([&counter]() {
            MainThreadQueue::instance().post([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        });
    }

    for (auto& t : workers) t.join();

    MainThreadQueue::instance().flush();

    REQUIRE(counter.load() == thread_count);
}

TEST_CASE("MainThreadQueue: flush executes all callables posted from a single worker thread",
          "[core][main_thread_queue]")
{
    drain();

    constexpr int post_count = 100;
    std::atomic<int> counter{0};

    std::thread worker([&counter, post_count]() {
        for (int i = 0; i < post_count; ++i) {
            MainThreadQueue::instance().post([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
    });
    worker.join();

    MainThreadQueue::instance().flush();

    REQUIRE(counter.load() == post_count);
}

// ---------------------------------------------------------------------------
// Exception safety
// ---------------------------------------------------------------------------

TEST_CASE("MainThreadQueue: exception thrown by callable is swallowed, subsequent callables still run",
          "[core][main_thread_queue]")
{
    // flush() is noexcept — exceptions from callables must not propagate.
    drain();

    int after_throw = 0;

    MainThreadQueue::instance().post([]() {
        throw std::runtime_error("intentional test exception");
    });
    MainThreadQueue::instance().post([&after_throw]() { ++after_throw; });

    REQUIRE_NOTHROW(MainThreadQueue::instance().flush());
    REQUIRE(after_throw == 1);
}

// ---------------------------------------------------------------------------
// Null callable guard
// ---------------------------------------------------------------------------

TEST_CASE("MainThreadQueue: posting a null callable is silently ignored",
          "[core][main_thread_queue]")
{
    drain();

    // post(nullptr / empty std::function) must not enqueue anything.
    REQUIRE_NOTHROW(MainThreadQueue::instance().post({}));

    // flush on an effectively empty queue must not crash.
    REQUIRE_NOTHROW(MainThreadQueue::instance().flush());
}
