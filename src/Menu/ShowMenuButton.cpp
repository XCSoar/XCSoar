/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "ShowMenuButton.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Layout.hpp"
#include "Input/InputEvents.hpp"
#include "Util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

bool
ShowMenuButton::OnClicked()
{
  InputEvents::ShowMenu();
  return true;
}

void
ShowMenuButton::OnPaint(Canvas &canvas)
{
  const unsigned width = canvas.GetWidth(), height = canvas.GetHeight();
  const unsigned pen_width = Layout::ScalePenWidth(2);
  const unsigned padding = Layout::GetTextPadding() + pen_width;

  canvas.Select(Pen(pen_width, COLOR_BLACK));
  canvas.DrawRoundRectangle(0, 0, width - 1, height - 1,
                            Layout::SmallScale(8), Layout::SmallScale(8));

  const RasterPoint m[] = {
    RasterPoint(padding, height - padding),
    RasterPoint(padding, padding),
    RasterPoint(width / 2, height - 2 * padding),
    RasterPoint(width - padding, padding),
    RasterPoint(width - padding, height - padding),
  };

  canvas.DrawPolyline(m, ARRAY_SIZE(m));

  if (IsDown()) {
#ifdef ENABLE_OPENGL
    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    canvas.DrawFilledRectangle(0, 0, canvas.GetWidth(), canvas.GetHeight(),
                               COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(0, 0, width, height);
#endif
  }
}
