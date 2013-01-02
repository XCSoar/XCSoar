/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

bool
TargetMapWindow::OnCancelMode()
{
  if (drag_mode != DRAG_NONE) {
    ReleaseCapture();
    drag_mode = DRAG_NONE;
  }

  return BufferWindow::OnCancelMode();
}

bool
TargetMapWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{
  // Ignore single click event if double click detected
  if (drag_mode != DRAG_NONE)
    return true;

  SetFocus();

  drag_start.x = x;
  drag_start.y = y;
  drag_last = drag_start;

  if (isClickOnTarget(drag_start)) {
    drag_mode = DRAG_TARGET;
    SetCapture();
    return true;
  } else if (isInSector(x, y)) {
    drag_mode = DRAG_OZ;
    SetCapture();
    PaintWindow::Invalidate();
    return true;
  }

  return false;
}

bool
TargetMapWindow::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (drag_mode != DRAG_NONE)
    ReleaseCapture();

  DragMode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  switch (old_drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET:
    TargetDragged(drag_last.x, drag_last.y);
    return true;

  case DRAG_OZ:
    TargetDragged(drag_last.x, drag_last.y);
    return true;
  }

  return false;
}

bool
TargetMapWindow::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  switch (drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET:
    if (isInSector(x, y)) {
      drag_last.x = x;
      drag_last.y = y;

      /* no full repaint: copy the map from the buffer, draw dragged
         icon on top */
      PaintWindow::Invalidate();
    }
    return true;

  case DRAG_OZ:
    if (manhattan_distance(drag_last, RasterPoint{x,y}) > Layout::GetHitRadius()) {
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
