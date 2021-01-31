/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TASK_MAP_BUTTON_RENDERER_HPP
#define XCSOAR_TASK_MAP_BUTTON_RENDERER_HPP

#include "Renderer/ButtonRenderer.hpp"
#include "ui/canvas/BufferCanvas.hpp"

struct MapLook;
class OrderedTask;

/**
 * A window that shows the task.
 */
class TaskMapButtonRenderer : public ButtonRenderer {
  const MapLook &look;

  /**
   * \todo This should not be store within this class. Instead this class
   *       should obtain this information from an authoritative source.
   */
  const OrderedTask *task;

  mutable BufferCanvas buffer;
  mutable PixelSize size;

public:
  explicit TaskMapButtonRenderer(const MapLook &_look)
    :look(_look), task(nullptr), size(0, 0) {}

  void SetTask(const OrderedTask *_task) {
    task = _task;
    InvalidateBuffer();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  bool enabled, bool focused, bool pressed) const override;

  void InvalidateBuffer() {
    size.width = 0;
  }

private:
  bool IsBufferValid(PixelSize new_size) const {
    return
#ifdef ENABLE_OPENGL
      buffer.IsDefined() &&
#endif
      size == new_size;
  }
};

#endif /* DLGTASKMANAGER_HPP */
