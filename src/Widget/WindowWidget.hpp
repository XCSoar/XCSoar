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

#ifndef XCSOAR_WINDOW_WIDGET_HPP
#define XCSOAR_WINDOW_WIDGET_HPP

#include "Widget.hpp"

class Window;

/**
 * A simple wrapper to turn a Window into a Widget.  The Window will
 * not be deleted in the destructor.
 */
class WindowWidget : public NullWidget {
  Window *window;

public:
  WindowWidget():window(nullptr) {}

  WindowWidget(Window *_window);

protected:
  bool IsDefined() const {
    return window != nullptr;
  }

  void SetWindow(Window *_window) {
    assert(window == nullptr);
    assert(_window != nullptr);

    window = _window;
  }

  /**
   * Deletes the window object
   */
  void DeleteWindow();

  const Window &GetWindow() const {
    assert(window != nullptr);
    return *window;
  }

  Window &GetWindow() {
    assert(window != nullptr);
    return *window;
  }

public:
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;
  virtual void Move(const PixelRect &rc) override;
};

#endif
