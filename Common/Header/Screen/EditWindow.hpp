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

#ifndef XCSOAR_SCREEN_EDIT_WINDOW_HXX
#define XCSOAR_SCREEN_EDIT_WINDOW_HXX

#include "Screen/Window.hpp"

/**
 * A simple text editor widget.
 */
class EditWindow : public Window {
public:
  void set(ContainerWindow &parent, int left, int top,
           unsigned width, unsigned height,
           bool multiline = false);

  /**
   * Temporary solution for Message.cpp.
   */
  void set_ro_ml(ContainerWindow &parent, int left, int top,
                 unsigned width, unsigned height);

  unsigned get_row_count() const {
    assert_none_locked();

#ifdef ENABLE_SDL
    return 1; // XXX
#else /* !ENABLE_SDL */
    return ::SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);
#endif /* !ENABLE_SDL */
  }

  void set_text(const TCHAR *text) {
    assert_none_locked();

#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    ::SetWindowText(hWnd, text);
#endif /* !ENABLE_SDL */
  }

  void get_text(TCHAR *text, size_t max_length) {
#ifdef ENABLE_SDL
    // XXX
    *text = 0;
#else /* !ENABLE_SDL */
    ::GetWindowText(hWnd, text, max_length);
#endif /* !ENABLE_SDL */
  }

  void set_read_only(bool value) {
    assert_none_locked();

#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    ::SendMessage(hWnd, EM_SETREADONLY, (WPARAM)(BOOL)value, 0L);
#endif /* !ENABLE_SDL */
  }

  void set_selection(int start, int end) {
    assert_none_locked();

#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    ::SendMessage(hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
#endif /* !ENABLE_SDL */
  }

  void set_selection() {
#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    set_selection(0, -1);
#endif /* !ENABLE_SDL */
  }
};

#endif
