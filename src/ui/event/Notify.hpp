// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_WINUSER
#include "ui/window/Window.hpp"
#endif

#include <atomic>
#include <functional>

namespace UI {

/**
 * This class implements message passing from any thread to the main
 * thread.
 */
class Notify final
#ifdef USE_WINUSER
  : Window
#endif
{
  std::atomic<bool> pending{false};

  using CallbackFunction = std::function<void()>;
  const CallbackFunction callback;

public:
  explicit Notify(CallbackFunction _callback) noexcept;

  Notify(const Notify &) = delete;

#ifndef USE_WINUSER
  ~Notify() noexcept {
    ClearNotification();
  }
#endif

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

#ifndef USE_WINUSER
  /**
   * Called by the event loop when the "notify" message is received.
   */
  static void Callback(void *ctx) noexcept;
#endif

#ifdef USE_WINUSER
private:
  bool OnUser(unsigned id) noexcept override;
#endif
};

} // namespace UI
