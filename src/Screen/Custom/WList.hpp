/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_WINDOW_LIST_HPP
#define XCSOAR_SCREEN_WINDOW_LIST_HPP

#include "Screen/Point.hpp"
#include "Compiler.h"

#include <list>

#include <assert.h>

class Window;
class Canvas;

/**
 * A container for more #Window objects.  It is used by the SDL/OpenGL
 * #ContainerWindow implementation to manage its children.
 */
class WindowList {
  std::list<Window *> list;

public:
  ~WindowList() {
    assert(list.empty());
  }

public:
  void Add(Window &w) {
    assert(!Contains(w));

    list.push_back(&w);
  }

  void Remove(Window &w) {
    assert(Contains(w));

    list.remove(&w);
  }

  /**
   * Remove and destroy all contained windows.
   */
  void Clear();

  gcc_pure
  bool Contains(const Window &w) const;

  /**
   * Is this window covered by a sibling?
   */
  gcc_pure
  bool IsCovered(const Window &w) const;

  void BringToTop(Window &w);
  void BringToBottom(Window &w);

  /**
   * Locate a window by its relative coordinates.
   */
  gcc_pure
  Window *FindAt(PixelScalar x, PixelScalar y);

  gcc_pure
  static Window *FindControl(std::list<Window*>::const_iterator i,
                             std::list<Window*>::const_iterator end);

  gcc_pure
  static Window *FindControl(std::list<Window*>::const_reverse_iterator i,
                             std::list<Window*>::const_reverse_iterator end);

  gcc_pure
  Window *FindFirstControl();

  gcc_pure
  Window *FindLastControl();

  gcc_pure
  Window *FindNextChildControl(Window *reference);

  gcc_pure
  Window *FindPreviousChildControl(Window *reference);

  void Paint(Canvas &canvas);
};

#endif
