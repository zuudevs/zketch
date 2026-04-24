#include <catch2/catch_test_macros.hpp>

#include "zk/event/event_dispatcher.hpp"
#include "zk/event/event_types.hpp"

using namespace zk::event;

// ---------------------------------------------------------------------------
// subscribe + dispatch
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: subscribe and dispatch calls handler with correct payload",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    ClickPayload received{};
    bool called = false;

    disp.subscribe<ClickPayload>(EventType::Click,
        [&](const ClickPayload& p) {
            called = true;
            received = p;
        });

    disp.dispatch(EventType::Click, ClickPayload{});

    REQUIRE(called);
}

TEST_CASE("EventDispatcher: ResizePayload carries correct size",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    zk::metric::Size<uint16_t> received_size{};

    disp.subscribe<ResizePayload>(EventType::Resize,
        [&](const ResizePayload& p) {
            received_size = p.size;
        });

    disp.dispatch(EventType::Resize, ResizePayload{ zk::metric::Size<uint16_t>{800u, 600u} });

    REQUIRE(received_size.w == 800u);
    REQUIRE(received_size.h == 600u);
}

TEST_CASE("EventDispatcher: KeyPayload carries correct vkey",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    uint32_t received_vkey = 0;

    disp.subscribe<KeyPayload>(EventType::KeyDown,
        [&](const KeyPayload& p) {
            received_vkey = p.vkey;
        });

    disp.dispatch(EventType::KeyDown, KeyPayload{ 0x41u }); // 'A'

    REQUIRE(received_vkey == 0x41u);
}

// ---------------------------------------------------------------------------
// unsubscribe
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: unsubscribe prevents handler from being called",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    int count = 0;
    auto id = disp.subscribe<ClickPayload>(EventType::Click,
        [&](const ClickPayload&) { ++count; });

    disp.dispatch(EventType::Click, ClickPayload{});
    REQUIRE(count == 1);

    disp.unsubscribe(EventType::Click, id);
    disp.dispatch(EventType::Click, ClickPayload{});

    REQUIRE(count == 1); // still 1 — handler was removed
}

TEST_CASE("EventDispatcher: unsubscribe with unknown id is a no-op",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    int count = 0;
    disp.subscribe<ClickPayload>(EventType::Click,
        [&](const ClickPayload&) { ++count; });

    // Unsubscribe a non-existent id — must not crash or affect other handlers
    REQUIRE_NOTHROW(disp.unsubscribe(EventType::Click, 9999u));

    disp.dispatch(EventType::Click, ClickPayload{});
    REQUIRE(count == 1);
}

// ---------------------------------------------------------------------------
// Multiple subscribers
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: multiple subscribers for one event type all receive dispatch",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    int a = 0, b = 0, c = 0;

    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { ++a; });
    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { ++b; });
    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { ++c; });

    disp.dispatch(EventType::Click, ClickPayload{});

    REQUIRE(a == 1);
    REQUIRE(b == 1);
    REQUIRE(c == 1);
}

TEST_CASE("EventDispatcher: handlers are called in subscription order",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    std::vector<int> order;

    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { order.push_back(1); });
    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { order.push_back(2); });
    disp.subscribe<ClickPayload>(EventType::Click, [&](const ClickPayload&) { order.push_back(3); });

    disp.dispatch(EventType::Click, ClickPayload{});

    REQUIRE(order == std::vector<int>{1, 2, 3});
}

// ---------------------------------------------------------------------------
// Dispatch with no handlers
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: dispatch with no handlers is a no-op",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    // No handlers registered — must not crash
    REQUIRE_NOTHROW(disp.dispatch(EventType::Close, ClosePayload{}));
    REQUIRE(disp.handler_count(EventType::Close) == 0);
}

// ---------------------------------------------------------------------------
// Exception safety
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: exception from handler is caught and does not propagate",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    int after_throw = 0;

    disp.subscribe<ClickPayload>(EventType::Click,
        [](const ClickPayload&) { throw std::runtime_error("handler error"); });

    disp.subscribe<ClickPayload>(EventType::Click,
        [&](const ClickPayload&) { ++after_throw; });

    // dispatch must not throw even though the first handler throws
    REQUIRE_NOTHROW(disp.dispatch(EventType::Click, ClickPayload{}));

    // The second handler must still have been called
    REQUIRE(after_throw == 1);
}

// ---------------------------------------------------------------------------
// handler_count
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: handler_count reflects subscribe/unsubscribe",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    REQUIRE(disp.handler_count(EventType::Resize) == 0);

    auto id1 = disp.subscribe<ResizePayload>(EventType::Resize,
        [](const ResizePayload&) {});
    REQUIRE(disp.handler_count(EventType::Resize) == 1);

    auto id2 = disp.subscribe<ResizePayload>(EventType::Resize,
        [](const ResizePayload&) {});
    REQUIRE(disp.handler_count(EventType::Resize) == 2);

    disp.unsubscribe(EventType::Resize, id1);
    REQUIRE(disp.handler_count(EventType::Resize) == 1);

    disp.unsubscribe(EventType::Resize, id2);
    REQUIRE(disp.handler_count(EventType::Resize) == 0);
}

// ---------------------------------------------------------------------------
// Independent event slots
// ---------------------------------------------------------------------------

TEST_CASE("EventDispatcher: handlers for different event types are independent",
          "[event][dispatcher]")
{
    EventDispatcher disp;

    int click_count = 0, resize_count = 0;

    disp.subscribe<ClickPayload>(EventType::Click,
        [&](const ClickPayload&) { ++click_count; });
    disp.subscribe<ResizePayload>(EventType::Resize,
        [&](const ResizePayload&) { ++resize_count; });

    disp.dispatch(EventType::Click, ClickPayload{});
    disp.dispatch(EventType::Click, ClickPayload{});
    disp.dispatch(EventType::Resize, ResizePayload{});

    REQUIRE(click_count  == 2);
    REQUIRE(resize_count == 1);
}
