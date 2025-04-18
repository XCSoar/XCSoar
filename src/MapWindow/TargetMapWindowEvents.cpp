// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TargetMapWindow.hpp"
#include "Screen/Layout.hpp"

void
TargetMapWindow::OnCancelMode() noexcept
{
  if (drag_mode != DRAG_NONE) {
    ReleaseCapture();
    drag_mode = DRAG_NONE;
  }

  BufferWindow::OnCancelMode();
}

bool
TargetMapWindow::OnMouseDown(PixelPoint p) noexcept
{
  // Ignore single click event if double click detected
  if (drag_mode != DRAG_NONE)
    return true;

  SetFocus();

  drag_start = p;
  drag_last = drag_start;

  if (isClickOnTarget(drag_start)) {
    drag_mode = isInSector(p)
      ? DRAG_TARGET
      : DRAG_TARGET_OUTSIDE;

    SetCapture();
    PaintWindow::Invalidate();
    return true;
  } else if (isInSector(p)) {
    drag_mode = DRAG_OZ;
    SetCapture();
    PaintWindow::Invalidate();
    return true;
  }

  return false;
}

bool
TargetMapWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  DragMode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  if (old_drag_mode != DRAG_NONE) {
    ReleaseCapture();
    PaintWindow::Invalidate();
  }

  switch (old_drag_mode) {
  case DRAG_NONE:
  case DRAG_TARGET_OUTSIDE:
    break;

  case DRAG_TARGET:
    TargetDragged(drag_last);
    return true;

  case DRAG_OZ:
    TargetDragged(drag_last);
    return true;
  }

  return false;
}

bool
TargetMapWindow::OnMouseMove(PixelPoint p,
                             [[maybe_unused]] unsigned keys) noexcept
{
  switch (drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET:
  case DRAG_TARGET_OUTSIDE:
    if (isInSector(p)) {
      drag_last = p;
      drag_mode = DRAG_TARGET;

      /* no full repaint: copy the map from the buffer, draw dragged
         icon on top */
      PaintWindow::Invalidate();
    }
    return true;

  case DRAG_OZ:
    if ((unsigned)ManhattanDistance(drag_last, p) > Layout::GetHitRadius()) {
      /* cancel the target move click when the finger has moved too
         far since it was pressed down */
      ReleaseCapture();
      drag_mode = DRAG_NONE;
      PaintWindow::Invalidate();
    }

    return true;
  }

  return false;
}
