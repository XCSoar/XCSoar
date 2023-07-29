// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskMapButtonRenderer.hpp"
#include "Gauge/TaskView.hpp"
#include "Look/MapLook.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

static void
DrawTask(Canvas &canvas, const PixelRect rc,
         const MapLook &look, const OrderedTask &task) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTask(canvas, rc, task,
            basic.GetLocationOrInvalid(),
            CommonInterface::GetMapSettings(),
            look.task, look.airspace, look.overlay,
            data_components->terrain.get(),
            data_components->airspaces.get(),
            true);
}

void
TaskMapButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                  ButtonState state) const noexcept
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

    DrawTask(buffer, PixelRect{new_size}, look, *task);

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

  if (state == ButtonState::PRESSED) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(rc, COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(rc);
#endif
  }
}
