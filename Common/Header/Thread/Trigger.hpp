/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#ifndef XCSOAR_THREAD_TRIGGER_HXX
#define XCSOAR_THREAD_TRIGGER_HXX

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * This class wraps an OS specific trigger.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Trigger {
  HANDLE handle;

public:
  /**
   * Initializes the trigger.
   *
   * @param name an application specific name for this trigger
   */
  Trigger(LPCTSTR name)
    :handle(::CreateEvent(NULL, true, false, name)) {}
  ~Trigger() {
    ::CloseHandle(handle);
  }

public:
  /**
   * Waits until this object is triggered with trigger().  If this
   * object is already triggered, this method returns immediately.
   *
   * @param timeout_ms the maximum number of milliseconds to wait
   * @return true if this object was triggered, false if the timeout
   * has expired
   */
  bool wait(unsigned timeout_ms) {
    if (::WaitForSingleObject(handle, timeout_ms) != WAIT_OBJECT_0)
      return false;
    ::ResetEvent(handle);
    return true;
  }

  /**
   * Checks if this object is triggered.
   * @return true if this object was triggered, false if not
   */
  bool test(void) {
    if (::WaitForSingleObject(handle, 0) != WAIT_OBJECT_0)
      return false;
    return true;
  }

  /**
   * Waits indefinitely until this object is triggered with trigger().
   * If this object is already triggered, this method returns
   * immediately.
   */
  void wait() {
    wait(INFINITE);
  }

  /**
   * Wakes up the thread waiting for the trigger.  The state of the
   * trigger is reset only if a thread was really woken up.
   */
  void trigger() {
#if defined(_WIN32_WCE) && defined(__MINGW32__) && defined(EVENT_SET)
    /* mingw32ce < 0.59 has a bugged SetEvent() implementation in
       kfuncs.h */
    ::EventModify(handle, EVENT_SET);
#else
    ::SetEvent(handle);
#endif
  }

  /**
   * Wakes up the thread waiting for the trigger, and immediately
   * resets the state of the trigger.
   */
  void pulse() {
    ::PulseEvent(handle);
  }
  /**
   * Resets the trigger
   */
  void reset() {
#if defined(_WIN32_WCE) && defined(__MINGW32__) && defined(EVENT_RESET)
    /* mingw32ce < 0.59 has a bugged SetEvent() implementation in
       kfuncs.h */
    ::EventModify(handle, EVENT_RESET);
#else
    ::ResetEvent(handle);
#endif
  }

};

#endif
