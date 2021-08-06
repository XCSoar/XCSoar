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

#ifndef XCSOAR_EVENT_NOTIFY_HPP
#define XCSOAR_EVENT_NOTIFY_HPP

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
  virtual bool OnUser(unsigned id) override;
#endif
};

} // namespace UI

#endif
