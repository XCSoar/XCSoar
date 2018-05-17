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

#include "TaskMapButtonRenderer.hpp"
#include "Gauge/TaskView.hpp"
#include "Look/MapLook.hpp"
#include "Interface.hpp"
#include "Components.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

static void
DrawTask(Canvas &canvas, const PixelRect rc,
         const MapLook &look, const OrderedTask &task)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTask(canvas, rc, task,
            basic.location_available ? basic.location : GeoPoint::Invalid(),
            CommonInterface::GetMapSettings(),
            look.task, look.airspace, look.overlay,
            terrain, &airspace_database,
            true);
}

void
TaskMapButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                  gcc_unused bool enabled,
                                  gcc_unused bool focused,
                                  bool pressed) const
{
  if (task == nullptr) {
    canvas.ClearWhite();
    return;
  }

  const PixelSize new_size = rc.GetSize();
  if (!IsBufferValid(new_size)) {
    if (!buffer.IsDefined()) {
#ifdef ENABLE_OPENGL
      buffer.Create(new_size);
#else
      buffer.Create(canvas, new_size);
#endif
    } else
      buffer.Grow(new_size);

    size = new_size;

#ifdef ENABLE_OPENGL
    buffer.Begin(canvas);
#endif

    const PixelRect buffer_rc(0, 0, new_size.cx, new_size.cy);
    DrawTask(buffer, buffer_rc, look, *task);

#ifdef ENABLE_OPENGL
    buffer.Commit(canvas);
#endif
  } else {
#ifdef ENABLE_OPENGL
    buffer.CopyTo(canvas);
#endif
  }

#ifndef ENABLE_OPENGL
  canvas.Copy(buffer);
#endif

  if (pressed) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}
