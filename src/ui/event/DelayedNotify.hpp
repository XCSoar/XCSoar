/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_EVENT_DELAYED_NOTIFY_HPP
#define XCSOAR_EVENT_DELAYED_NOTIFY_HPP

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

#endif
