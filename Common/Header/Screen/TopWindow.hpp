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

#ifndef XCSOAR_SCREEN_TOP_WINDOW_HXX
#define XCSOAR_SCREEN_TOP_WINDOW_HXX

#include "Screen/ContainerWindow.hpp"

#if defined(WIN32) && !defined(WINDOWSPC) && \
  !defined(GNAV) && (UNDER_CE >= 300 || _WIN32_WCE >= 0x0300)
#define HAVE_ACTIVATE_INFO
#endif

#ifdef HAVE_ACTIVATE_INFO
#include <aygshell.h>
#endif

#ifdef ENABLE_SDL
class TopCanvas : public Canvas {
public:
  void set();

  void full_screen();
};
#endif

/**
 * A top-level full-screen window.
 */
class TopWindow : public ContainerWindow {
#ifdef ENABLE_SDL
  TopCanvas screen;
#else /* !ENABLE_SDL */
#ifdef HAVE_ACTIVATE_INFO
  SHACTIVATEINFO s_sai;
#endif
#endif /* !ENABLE_SDL */

public:
  TopWindow();

  static bool find(LPCTSTR cls, LPCTSTR text);

  void set(LPCTSTR cls, LPCTSTR text,
           int left, int top, unsigned width, unsigned height);

  void set_active() {
#ifdef ENABLE_SDL
    // XXX
#else
    ::SetActiveWindow(hWnd);
#endif
  }

  void full_screen();

#ifdef ENABLE_SDL
  virtual void expose(const RECT &rect);
  virtual void expose();
#endif /* ENABLE_SDL */

  void close() {
#ifdef ENABLE_SDL
    on_close();
#else /* ENABLE_SDL */
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
#endif
  }

protected:
  virtual bool on_activate();
  virtual bool on_deactivate();

#ifdef ENABLE_SDL
  virtual bool on_event(const SDL_Event &event);
#else /* !ENABLE_SDL */
  virtual LRESULT on_message(HWND _hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif /* !ENABLE_SDL */

public:
  /**
   * Runs the event loop until the application quits.
   */
  int event_loop(unsigned accelerators_id);
};

#endif
