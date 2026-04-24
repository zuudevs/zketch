#pragma once

#include "zk/event/event_types.hpp"
#include "zk/log/logger.hpp"
#include <any>
#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

// Forward-declare thread-id assertion helper (defined in event_dispatcher.cpp)
namespace zk::event::detail {
void assert_main_thread() noexcept;
} // namespace zk::event::detail

namespace zk::event {

/// Opaque identifier returned by subscribe(); pass to unsubscribe() to remove
/// a specific handler.  Zero is reserved as "invalid".
using HandlerId = uint32_t;

/// Central event dispatcher.
///
/// Design constraints (Req 18.1–18.9):
///   - O(1) lookup via std::array indexed by EventType (no per-dispatch alloc)
///   - Multiple subscribers per event type (Req 18.4)
///   - Deterministic call order: handlers are invoked in subscription order (Req 18.9)
///   - Dispatch must run on Main_Thread; debug-mode assertion enforces this (Req 18.6)
///   - Exceptions from handlers are caught, logged, and swallowed — no crash (Req 9.6)
///   - Dispatch with no handlers is a no-op (Req 18.8)
///
/// Usage:
/// @code
///   EventDispatcher disp;
///   auto id = disp.subscribe<ClickPayload>(EventType::Click,
///       [](const ClickPayload&) { /* ... */ });
///   disp.dispatch(EventType::Click, ClickPayload{});
///   disp.unsubscribe(EventType::Click, id);
/// @endcode
class EventDispatcher {
public:
    EventDispatcher() noexcept = default;

    // Non-copyable, movable
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) noexcept = default;
    EventDispatcher& operator=(EventDispatcher&&) noexcept = default;

    ~EventDispatcher() noexcept = default;

    // -----------------------------------------------------------------------
    // subscribe<Payload>
    //
    // Registers a handler for the given event type.  The handler will be
    // called with a const reference to Payload whenever dispatch() is invoked
    // for that event type.
    //
    // Returns a HandlerId that can be passed to unsubscribe().
    // Thread: must be called from Main_Thread (assertion in debug mode).
    // -----------------------------------------------------------------------
    template <typename Payload>
    [[nodiscard]] HandlerId subscribe(
        EventType type,
        std::function<void(const Payload&)> handler)
    {
        detail::assert_main_thread();

        const auto idx = to_index(type);
        const HandlerId id = next_id_++;

        handlers_[idx].emplace_back(id, std::any{ std::move(handler) });
        return id;
    }

    // -----------------------------------------------------------------------
    // unsubscribe
    //
    // Removes the handler identified by `id` from the given event type's list.
    // If `id` is not found the call is a no-op.
    // Thread: must be called from Main_Thread (assertion in debug mode).
    // -----------------------------------------------------------------------
    void unsubscribe(EventType type, HandlerId id) noexcept;

    // -----------------------------------------------------------------------
    // dispatch<Payload>
    //
    // Invokes all handlers registered for `type` with the given payload.
    // Handlers are called in subscription order.
    // Exceptions thrown by handlers are caught, logged via ZK_LOG, and
    // execution continues with the next handler (Req 9.6).
    // Thread: must be called from Main_Thread (assertion in debug mode).
    // -----------------------------------------------------------------------
    template <typename Payload>
    void dispatch(EventType type, const Payload& payload) noexcept
    {
        detail::assert_main_thread();

        const auto idx = to_index(type);
        for (auto& [id, any_handler] : handlers_[idx]) {
            try {
                auto* fn = std::any_cast<std::function<void(const Payload&)>>(&any_handler);
                if (fn && *fn) {
                    (*fn)(payload);
                }
            } catch (const std::exception& ex) {
                ZK_LOG(zk::log::Level::Error, zk::log::Domain::Event,
                       ex.what());
            } catch (...) {
                ZK_LOG(zk::log::Level::Error, zk::log::Domain::Event,
                       "Unknown exception in event handler");
            }
        }
    }

    /// Returns the number of handlers currently registered for `type`.
    [[nodiscard]] std::size_t handler_count(EventType type) const noexcept;

private:
    // Number of distinct event types (uses the _Count sentinel)
    static constexpr std::size_t kEventCount =
        static_cast<std::size_t>(EventType::_Count);

    // Each slot holds a list of (HandlerId, type-erased handler) pairs.
    // std::any stores std::function<void(const Payload&)>.
    using HandlerEntry = std::pair<HandlerId, std::any>;
    using HandlerList  = std::vector<HandlerEntry>;

    std::array<HandlerList, kEventCount> handlers_{};
    HandlerId next_id_{ 1 };  // 0 is reserved as "invalid"

    // Converts EventType to array index; undefined for _Count.
    static constexpr std::size_t to_index(EventType type) noexcept {
        return static_cast<std::size_t>(type);
    }
};

} // namespace zk::event
