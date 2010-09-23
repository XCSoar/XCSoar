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

#ifndef XCSOAR_SCREEN_CHECK_BOX_WINDOW_HXX
#define XCSOAR_SCREEN_CHECK_BOX_WINDOW_HXX

#include "Screen/Window.hpp"

class CheckBoxStyle : public WindowStyle {
public:
  CheckBoxStyle() {
#ifndef ENABLE_SDL
    style |= BS_CHECKBOX | BS_AUTOCHECKBOX;
#endif
  }

  CheckBoxStyle(const WindowStyle _style):WindowStyle(_style) {
#ifndef ENABLE_SDL
    style |= BS_CHECKBOX | BS_AUTOCHECKBOX;
#endif
  }

  void enable_custom_painting() {
    WindowStyle::enable_custom_painting();
#ifndef ENABLE_SDL
    style |= BS_OWNERDRAW;
#endif
  }
};

#ifdef ENABLE_SDL

#include "Screen/PaintWindow.hpp"

class CheckBox : public PaintWindow {
public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           int left, int top, unsigned width, unsigned height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    // XXX
    PaintWindow::set(parent, left, top, width, height);
  }

  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    // XXX
    PaintWindow::set(parent, left, top, width, height);
  }

  bool get_checked() const {
    return false; // XXX
  }

  void set_checked(bool value) {
    // XXX
  }
};

#else /* !ENABLE_SDL */

#include "Screen/ButtonWindow.hpp"

/**
 * A check box.
 */
class CheckBox : public BaseButtonWindow {
public:
  void set(ContainerWindow &parent, const TCHAR *text, unsigned id,
           int left, int top, unsigned width, unsigned height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::set(parent, text, id, left, top, width, height, style);
  }

  void set(ContainerWindow &parent, const TCHAR *text,
           int left, int top, unsigned width, unsigned height,
           const CheckBoxStyle style=CheckBoxStyle()) {
    BaseButtonWindow::set(parent, text, left, top, width, height, style);
  }

  bool get_checked() const {
    return SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
  }

  void set_checked(bool value) {
    SendMessage(hWnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
  }
};

#endif

#endif
