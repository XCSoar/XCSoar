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

#ifndef XCSOAR_GLUE_MAP_WINDOW_HPP
#define XCSOAR_GLUE_MAP_WINDOW_HPP

#include "MapWindow.hpp"
#include "PeriodClock.hpp"
#include "GestureManager.hpp"

class GlueMapWindow : public MapWindow {
  unsigned idle_robin;

  PeriodClock mouse_down_clock;

public:
  GlueMapWindow();

  void QuickRedraw(const SETTINGS_MAP &_settings_map);

  bool Idle();

private:
  enum drag_mode {
    DRAG_NONE,
    DRAG_PAN,
    DRAG_GESTURE,
    DRAG_SIMULATOR,
    DRAG_TARGET,
  } drag_mode;

  GeoPoint drag_start_geopoint;
  POINT drag_start, drag_last, drag_last_valid_target;
  GestureManager gestures;
  bool ignore_single_click;

  /**
   * The projection which was active when dragging started.
   */
  Projection drag_projection;

  bool AirspaceDetailsAtPoint(const GeoPoint &location) const;

protected:
  // events
  virtual bool on_mouse_double(int x, int y);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_wheel(int delta);
  /**
   * This event handler gets called when a gesture has
   * been painted by the user
   * @param gesture The gesture string (e.g. "ULR")
   * @return True if the gesture was handled by the
   * event handler, False otherwise
   */
  bool on_mouse_gesture(const char* gesture);

#if defined(GNAV)
  virtual bool on_key_down(unsigned key_code);
#else
  virtual bool on_key_up(unsigned key_code);
#endif

  virtual bool on_setfocus();
  virtual bool on_cancel_mode();
  virtual void on_paint(Canvas &canvas);

private:

  /**
   * This (non-virtual, non-inherited) method gets called by either
   * on_key_down() (Altair and PNAs) or on_key_up() (all other
   * platforms).
   *
   * Some PDAs like iPAQ hx4700 send 0xca..0xcd in WM_KEYDOWN, but
   * 0xc0..0xc4 (VK_APP1..4) in WM_KEYUP.  We prefer the VK_APP codes.
   */
  bool on_key_press(unsigned key_code);
};

#endif
