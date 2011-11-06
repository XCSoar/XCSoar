/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FINAL_GLIDE_BAR_RENDERER_HPP
#define XCSOAR_FINAL_GLIDE_BAR_RENDERER_HPP

#include "Screen/Point.hpp"

class Canvas;
struct DerivedInfo;
struct TaskLook;

class FinalGlideBarRenderer {
  const TaskLook &task_look;

public:
  FinalGlideBarRenderer(const TaskLook &_task_look)
    :task_look(_task_look) {}

  void Draw(Canvas &canvas, const PixelRect &rc,
            const DerivedInfo &calculated) const;
};

#endif
