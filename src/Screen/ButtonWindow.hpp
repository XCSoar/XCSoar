/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_BUTTON_WINDOW_HXX
#define XCSOAR_SCREEN_BUTTON_WINDOW_HXX

#ifdef USE_GDI

#include "Screen/Window.hpp"
#include "Util/tstring.hpp"

#include <tchar.h>

/**
 * A base class for WC_BUTTON windows.
 */
class BaseButtonWindow : public Window {
public:
    /**
     * On WIN32, a WM_COMMAND/BN_CLICKED message with this id will be
     * bounced back to the originating child
     * ButtonWindow::OnClicked().
     */
  static constexpr unsigned COMMAND_BOUNCE_ID = 0xbeef;

public:
  void Create(ContainerWindow &parent, tstring::const_pointer text,
              unsigned id,
              const PixelRect &rc,
              const WindowStyle style);

  void Create(ContainerWindow &parent, tstring::const_pointer text,
              const PixelRect &rc,
              const WindowStyle style) {
    Create(parent, text, COMMAND_BOUNCE_ID, rc, style);
  }

  gcc_pure
  unsigned GetID() const {
    return ::GetWindowLong(hWnd, GWL_ID);
  }

  void SetID(unsigned _id) {
    ::SetWindowLong(hWnd, GWL_ID, _id);
  }

  /**
   * The button was clicked, and its action shall be triggered.
   */
  virtual bool OnClicked();

protected:
  /**
   * Synthesise a click.
   */
  void Click();
};

#endif /* USE_GDI */

#endif
