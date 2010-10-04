/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_DRAW_THREAD_HPP
#define XCSOAR_DRAW_THREAD_HPP

#include "Thread/StoppableThread.hpp"
#include "Thread/Trigger.hpp"

class GlueMapWindow;
class GaugeFLARM;
class GaugeThermalAssistant;

/**
 * The DrawThread handles the rendering and drawing on the screen.
 * The Map and GaugeFLARM both are triggered on GPS updates synchronously, 
 * which is why they are both handled by this thread.  The GaugeVario is
 * triggered on vario data which may be faster than GPS updates, which is
 * why it is not handled by this thread.
 * 
 */
class DrawThread : public StoppableThread {
  enum {
    MIN_WAIT_TIME = 100,
  };

  /**
   * The drawing thread runs while this trigger is set.
   */
  Trigger running;

  /**
   * This triggers a redraw.
   */
  Trigger trigger;

  /** Pointer to the MapWindow */
  GlueMapWindow &map;

  /** Pointer to the FLARM gauge */
  GaugeFLARM *flarm;

  /** Pointer to the FLARM gauge */
  GaugeThermalAssistant *ta;

public:
  DrawThread(GlueMapWindow &_map, GaugeFLARM *_flarm,
             GaugeThermalAssistant *_ta)
    :running(_T("DrawThread::running"), true),
     trigger(_T("DrawThread::trigger"), true),
     map(_map), flarm(_flarm), ta(_ta) {
  }

  /** Locks the Mutex and "pauses" the drawing thread */
  void suspend() {
    running.reset();
  }

  /** Releases the Mutex and "continues" the drawing thread */
  void resume() {
    running.trigger();
  }

  /**
   * To be removed, only used by GlueMapWindow::Idle().
   */
  bool is_triggered() {
    return trigger.test();
  }

  /**
   * Triggers a redraw.
   */
  void trigger_redraw() {
    trigger.trigger();
  }

  /**
   * Triggers thread shutdown.  Call join() after this to wait
   * synchronously for the thread to exit.
   */
  void stop() {
    StoppableThread::stop();
    trigger_redraw();
    resume();
  }

private:
  /**
   * Exchanges blackboard data between the device (device_blackboard)
   * and the #GlueMapWindow.
   */
  void ExchangeBlackboard();

protected:
  virtual void run();
};

#endif
