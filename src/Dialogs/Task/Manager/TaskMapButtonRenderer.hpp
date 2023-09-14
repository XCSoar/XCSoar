// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  explicit TaskMapButtonRenderer(const MapLook &_look) noexcept
    :look(_look), task(nullptr), size(0, 0) {}

  void SetTask(const OrderedTask *_task) noexcept {
    task = _task;
    InvalidateBuffer();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;

  void InvalidateBuffer() noexcept {
    size.width = 0;
  }

private:
  bool IsBufferValid(PixelSize new_size) const noexcept {
    return
#ifdef ENABLE_OPENGL
      buffer.IsDefined() &&
#endif
      size == new_size;
  }
};
