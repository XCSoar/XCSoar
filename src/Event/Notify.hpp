/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Util/NonCopyable.hpp"

#ifdef USE_GDI
#include "Screen/Window.hpp"
#endif

#include <atomic>

/**
 * This class implements message passing from any thread to the main
 * thread.  To use it, subclass it and implement the abstract method
 * OnNotification().
 */
class Notify : private
#ifndef USE_GDI
               NonCopyable
#else
               Window
#endif
{
  std::atomic<bool> pending;

#if defined(ANDROID) || defined(USE_CONSOLE)
  friend class EventLoop;
#elif defined(ENABLE_SDL)
  friend class EventLoop;
#endif

public:
  Notify();
  ~Notify();

  /**
   * Send a notification to this object.  This method can be called
   * from any thread.
   */
  void SendNotification();

  /**
   * Clear any pending notification.
   */
  void ClearNotification();

private:
  void RunNotification();

  /**
   * Called by the event loop when the "notify" message is received.
   */
  static void Callback(void *ctx);

protected:
  /**
   * Called after SendNotification() has been called at least once.
   * This method runs in the main thread.
   */
  virtual void OnNotification() = 0;

#ifdef USE_GDI
  virtual bool OnUser(unsigned id) override;
#endif
};

#endif
