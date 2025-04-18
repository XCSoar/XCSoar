// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalAssistantWindow.hpp"
#include "Look/ThermalAssistantLook.hpp"
#include "ui/canvas/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

ThermalAssistantWindow::ThermalAssistantWindow(const ThermalAssistantLook &_look,
                                               unsigned _padding, bool _small,
                                               [[maybe_unused]] bool _transparent) noexcept
  :renderer(_look, _padding, _small)
#ifdef ENABLE_OPENGL
  , transparent(_transparent)
#endif
{}

void
ThermalAssistantWindow::Update(const AttitudeState &attitude,
                               const DerivedInfo &derived) noexcept
{
  renderer.Update(attitude, derived);
  Invalidate();
}

void
ThermalAssistantWindow::OnResize(PixelSize new_size) noexcept
{
  AntiFlickerWindow::OnResize(new_size);

  renderer.UpdateLayout(GetClientRect());
}

void
ThermalAssistantWindow::DrawCircle(Canvas &canvas) noexcept
{
  canvas.DrawCircle(renderer.GetMiddle(), renderer.GetRadius());
}

void
ThermalAssistantWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  if (transparent) {
    const ScopeAlphaBlend alpha_blend;

    canvas.Select(renderer.GetLook().inner_circle_pen);
    canvas.Select(Brush(renderer.GetLook().background_color.WithAlpha(0xd0)));
    DrawCircle(canvas);
  } else
#endif
    canvas.Clear(renderer.GetLook().background_color);

  renderer.Paint(canvas);
}
