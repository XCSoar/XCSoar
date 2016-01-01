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

#ifndef XCSOAR_EVENT_MERGE_MOUSE_HPP
#define XCSOAR_EVENT_MERGE_MOUSE_HPP

#include "../../Shared/RotatePointer.hpp"

#include <assert.h>

class IOLoop;
struct Event;

/**
 * This class keeps track of the current mouse position and generates
 * movement events for the #EventQueue.  It is responsible for merging
 * the input of multiple (hot-plugged) mouse devices.
 */
class MergeMouse final {
  RotatePointer rotate;

  unsigned x, y;

  /**
   * Relative mouse wheel movement since last Generate() call.
   */
  int wheel;

  bool down;

  bool moved, pressed, released;

  /**
   * The number of pointer input devices.
   */
  unsigned n_pointers;

public:
  MergeMouse()
    :x(0), y(0), wheel(0),
     down(false), moved(false), pressed(false), released(false),
     n_pointers(0) {}

  MergeMouse(const MergeMouse &) = delete;

  ~MergeMouse() {
    assert(n_pointers == 0);
  }

  void SetScreenSize(unsigned width, unsigned height);

  void SetSwap(bool _swap) {
    rotate.SetSwap(_swap);
  }

  void SetInvert(bool _invert_x, bool _invert_y) {
    rotate.SetInvert(_invert_x, _invert_y);
  }

  void AddPointer() {
    ++n_pointers;
  }

  void RemovePointer() {
    assert(n_pointers > 0);

    --n_pointers;
  }

  bool HasPointer() const {
    return n_pointers > 0;
  }

  void SetDown(bool new_down);
  void MoveAbsolute(int new_x, int new_y);
  void MoveAbsolute(int new_x, int new_y,
                    int min_x, int max_x, int min_y, int max_y);
  void MoveRelative(int dx, int dy);

  void MoveWheel(int d) {
    wheel += d;
  }

  unsigned GetX() const {
    return x;
  }

  unsigned GetY() const {
    return y;
  }

  Event Generate();
};

#endif
