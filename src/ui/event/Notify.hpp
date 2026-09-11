// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <atomic>
#include <functional>

namespace UI {

/**
 * This class implements message passing from any thread to the main
 * thread.
 */
class Notify final
{
  std::atomic<bool> pending{false};

  using CallbackFunction = std::function<void()>;
  const CallbackFunction callback;

public:
  explicit Notify(CallbackFunction _callback) noexcept;

  Notify(const Notify &) = delete;

  ~Notify() noexcept {
    ClearNotification();
  }

  /**
   * Send a notification to this object.  This method can be called
   * from any thread.
   */
  void SendNotification() noexcept;

  /**
   * Clear any pending notification.
   */
  void ClearNotification() noexcept;

private:
  void RunNotification() noexcept;

  /**
   * Called by the event loop when the "notify" message is received.
   */
  static void Callback(void *ctx) noexcept;
};

} // namespace UI
