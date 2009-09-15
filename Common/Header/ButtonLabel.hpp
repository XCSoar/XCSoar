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

#ifndef BUTTON_LABEL_HPP
#define BUTTON_LABEL_HPP

#include "Screen/TextWindow.hpp"
#include "Interface.hpp"

class ContainerWindow;

class MenuButton : public TextWindow {
  bool enabled;

public:
  void set(ContainerWindow &parent,
           int left, int top, unsigned width, unsigned height,
           bool visible) {
    TextWindow::set(parent, left, top, width, height,
                    true, true, visible, true, true);
    Window::set_enabled(true);
    set_enabled(false);
    install_wndproc();
  }

  bool is_enabled() const {
    return enabled;
  }

  void set_enabled(bool _enabled) {
    enabled = _enabled;
  }

  virtual bool on_mouse_down(int x, int y);
};

class ButtonLabel: public ActionInterface {
 public:
  enum {
    NUMBUTTONLABELS = 16
  };

  static unsigned ButtonLabelGeometry;
  static MenuButton hWndButtonWindow[NUMBUTTONLABELS];
  static bool ButtonVisible[NUMBUTTONLABELS];

protected:
  static void GetButtonPosition(unsigned i, RECT rc,
				int *x, int *y,
				int *sizex, int *sizey);

public:
  static void CreateButtonLabels(ContainerWindow &parent, const RECT rc);
  static void AnimateButton(unsigned i);
  static void SetFont(const Font &Font);
  static void Destroy();
  static void SetLabelText(unsigned i, const TCHAR *text);
  static int Find(const Window &window);

  static bool ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size);
};

#endif
