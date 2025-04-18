// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Notify.hpp"
#include "Timer.hpp"

namespace UI {

/**
 * This class is similar to #Notify, but it delivers the notification
 * with a certain delay, to limit the rate of redundant notifications.
 */
class DelayedNotify final {
  Timer timer{[this]{ callback(); }};
  Notify notify{[this]{ timer.SchedulePreserve(delay); }};

  const std::chrono::steady_clock::duration delay;

  using Callback = std::function<void()>;
  const Callback callback;

public:
  explicit DelayedNotify(std::chrono::steady_clock::duration _delay,
                         Callback &&_callback) noexcept
    :delay(_delay), callback(std::move(_callback)) {}

  /**
   * Send a notification to this object.  This method can be called
   * from any thread.
   */
  void SendNotification() {
    notify.SendNotification();
  }
};

} // namespace UI
