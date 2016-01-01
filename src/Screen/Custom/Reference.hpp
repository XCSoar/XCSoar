/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_SDL_REFERENCE_HPP
#define XCSOAR_SCREEN_SDL_REFERENCE_HPP

#include "Util/StaticArray.hxx"

#include <cassert>

/**
 * A stable reference to a Window.  "Stable" means it notices when the
 * referenced Window has been destroyed.  It is used to remeber which
 * Window was focused before a new dialog was opened on top.
 */
class WindowReference {
  StaticArray<const ContainerWindow*, 6> parents;
  Window *window;

public:
  /**
   * Construct an empty reference, that points to no Window.
   */
  WindowReference():window(nullptr) {}

  WindowReference(const ContainerWindow &root, Window &_window)
    :window(&_window) {
    const ContainerWindow *parent = window->GetParent();
    while (true) {
      if (parent == &root)
        return;

      if (parent == nullptr || parents.full()) {
        window = nullptr;
        return;
      }

      parents.append(parent);
      parent = parent->GetParent();
    }
  }

  bool Defined() const {
    return window != nullptr;
  }

  /**
   * Check if the referenced Window still exists, and return it.
   * Returns nullptr if the referenced Window does not exist anymore.
   */
  Window *Get(const ContainerWindow &root) const {
    assert(window != nullptr);

    const ContainerWindow *parent = &root;
    for (int i = parents.size() - 1; i >= 0; --i) {
      const ContainerWindow &current = *parents[i];
      if (!parent->HasChild(current))
        return nullptr;

      parent = &current;
    }

    if (!parent->HasChild(*window))
      return nullptr;

    return window;
  }
};

#endif
