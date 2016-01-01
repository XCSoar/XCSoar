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

#include "ThermalAssistantWindow.hpp"
#include "Look/ThermalAssistantLook.hpp"
#include "Screen/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

ThermalAssistantWindow::ThermalAssistantWindow(const ThermalAssistantLook &_look,
                                               unsigned _padding, bool _small,
                                               bool _transparent)
  :renderer(_look, _padding, _small)
#ifdef ENABLE_OPENGL
  , transparent(_transparent)
#endif
{}

void
ThermalAssistantWindow::Update(const AttitudeState &attitude,
                               const DerivedInfo &derived)
{
  renderer.Update(attitude, derived);
  Invalidate();
}

void
ThermalAssistantWindow::OnResize(PixelSize new_size)
{
  AntiFlickerWindow::OnResize(new_size);

  renderer.UpdateLayout(GetClientRect());
}

void
ThermalAssistantWindow::DrawCircle(Canvas &canvas)
{
  canvas.DrawCircle(renderer.GetMiddle().x, renderer.GetMiddle().y,
                    renderer.GetRadius());
}

void
ThermalAssistantWindow::OnPaintBuffer(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  if (transparent) {
    const ScopeAlphaBlend alpha_blend;

    canvas.SelectBlackPen();
    canvas.Select(Brush(COLOR_WHITE.WithAlpha(0xd0)));
    DrawCircle(canvas);
  } else
#endif
    canvas.Clear(renderer.GetLook().background_color);

  renderer.Paint(canvas);
}
