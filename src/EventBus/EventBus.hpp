// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/event/Notify.hpp"
#include "thread/Mutex.hxx"

#include <functional>
#include <list>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Event {

/**
 * A type-safe publish/subscribe event bus for decoupling components.
 *
 * Components can subscribe to specific event types and receive
 * notifications when those events are posted. Events can be posted
 * from any thread; subscribers are called on the posting thread
 * unless PostToMain() is used.
 *
 * Usage:
 *   EventBus bus;
 *   auto sub = bus.Subscribe<MyEvent>([](const MyEvent &e) { ... });
 *   bus.Post(MyEvent{...});
 */
class EventBus {
public:
  /**
   * RAII subscription handle. Unsubscribes when destroyed.
   */
  class Subscription {
    friend class EventBus;
    EventBus *bus = nullptr;
    std::type_index type{typeid(void)};
    void *handler_ptr = nullptr;

    Subscription(EventBus *_bus, std::type_index _type, void *_ptr) noexcept
      : bus(_bus), type(_type), handler_ptr(_ptr) {}

  public:
    Subscription() noexcept = default;
    ~Subscription() noexcept { Unsubscribe(); }

    Subscription(const Subscription &) = delete;
    Subscription &operator=(const Subscription &) = delete;

    Subscription(Subscription &&other) noexcept
      : bus(other.bus), type(other.type), handler_ptr(other.handler_ptr) {
      other.bus = nullptr;
      other.handler_ptr = nullptr;
    }

    Subscription &operator=(Subscription &&other) noexcept {
      Unsubscribe();
      bus = other.bus;
      type = other.type;
      handler_ptr = other.handler_ptr;
      other.bus = nullptr;
      other.handler_ptr = nullptr;
      return *this;
    }

    void Unsubscribe() noexcept;
    [[nodiscard]] bool IsActive() const noexcept { return bus != nullptr; }
  };

private:
  struct HandlerBase {
    virtual ~HandlerBase() = default;
  };

  template<typename Event>
  struct Handler : HandlerBase {
    std::function<void(const Event &)> callback;
    explicit Handler(std::function<void(const Event &)> cb)
      : callback(std::move(cb)) {}
  };

  mutable Mutex mutex;
  std::unordered_map<std::type_index, std::list<std::unique_ptr<HandlerBase>>> handlers;

  struct PendingMainEvent {
    std::function<void()> callback;
  };
  std::vector<PendingMainEvent> pending_main_events;
  UI::Notify main_thread_notify{[this]{ ProcessMainEvents(); }};

public:
  EventBus() = default;
  ~EventBus() = default;

  EventBus(const EventBus &) = delete;
  EventBus &operator=(const EventBus &) = delete;

  /**
   * Subscribe to events of type Event.
   * Returns a Subscription that unsubscribes when destroyed.
   */
  template<typename Event>
  [[nodiscard]]
  Subscription Subscribe(std::function<void(const Event &)> callback) {
    const std::lock_guard lock{mutex};
    auto type = std::type_index(typeid(Event));
    auto handler = std::make_unique<Handler<Event>>(std::move(callback));
    void *ptr = handler.get();
    handlers[type].push_back(std::move(handler));
    return Subscription{this, type, ptr};
  }

  /**
   * Post an event to all subscribers. Called on the posting thread.
   * Thread-safe.
   */
  template<typename Event>
  void Post(const Event &event) {
    const std::lock_guard lock{mutex};
    auto type = std::type_index(typeid(Event));
    auto it = handlers.find(type);
    if (it == handlers.end())
      return;

    for (auto &h : it->second) {
      auto *typed = static_cast<Handler<Event> *>(h.get());
      typed->callback(event);
    }
  }

  /**
   * Post an event to be delivered on the main (UI) thread.
   * Thread-safe. Use when subscribers need to update UI.
   */
  template<typename Event>
  void PostToMain(Event event) {
    {
      const std::lock_guard lock{mutex};
      pending_main_events.push_back({[this, e = std::move(event)]() mutable {
        Post(e);
      }});
    }
    main_thread_notify.SendNotification();
  }

private:
  void ProcessMainEvents() noexcept {
    std::vector<PendingMainEvent> events;
    {
      const std::lock_guard lock{mutex};
      events = std::move(pending_main_events);
      pending_main_events.clear();
    }

    for (auto &pe : events) {
      try {
        pe.callback();
      } catch (...) {
      }
    }
  }

  void RemoveHandler(std::type_index type, void *ptr) noexcept {
    const std::lock_guard lock{mutex};
    auto it = handlers.find(type);
    if (it == handlers.end())
      return;

    auto &list = it->second;
    list.remove_if([ptr](const auto &h) { return h.get() == ptr; });
  }
};

inline void
EventBus::Subscription::Unsubscribe() noexcept
{
  if (bus != nullptr) {
    bus->RemoveHandler(type, handler_ptr);
    bus = nullptr;
    handler_ptr = nullptr;
  }
}

} // namespace Event
