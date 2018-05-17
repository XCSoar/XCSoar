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

#include "TargetMapWindow.hpp"
#include "Screen/Layout.hpp"

void
TargetMapWindow::OnCancelMode()
{
  if (drag_mode != DRAG_NONE) {
    ReleaseCapture();
    drag_mode = DRAG_NONE;
  }

  BufferWindow::OnCancelMode();
}

bool
TargetMapWindow::OnMouseDown(PixelPoint p)
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
TargetMapWindow::OnMouseUp(PixelPoint p)
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
TargetMapWindow::OnMouseMove(PixelPoint p, unsigned keys)
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
