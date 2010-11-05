/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "GlueMapWindow.hpp"

void
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, Color(50, 50, 50));
  canvas.select(dash_pen);

  const POINT Orig_Screen = render_projection.GetScreenOrigin();

  canvas.line(Orig_Screen.x + 20, Orig_Screen.y,
              Orig_Screen.x - 20, Orig_Screen.y);
  canvas.line(Orig_Screen.x, Orig_Screen.y + 20,
              Orig_Screen.x, Orig_Screen.y - 20);
}
