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

#include "MapWindow.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Computer/GlideComputer.hpp"

void
MapWindow::RenderTrail(Canvas &canvas, const PixelPoint aircraft_pos)
{
  unsigned min_time = std::max(0, (int)Basic().time - 600);
  DrawTrail(canvas, aircraft_pos, min_time);
}

void
MapWindow::DrawTrail(Canvas &canvas, const PixelPoint aircraft_pos,
                     unsigned min_time, bool enable_traildrift)
{
  if (glide_computer)
    trail_renderer.Draw(canvas, glide_computer->GetTraceComputer(),
                        render_projection, min_time,
                        enable_traildrift, aircraft_pos,
                        Basic(), Calculated(), GetMapSettings().trail);
}
