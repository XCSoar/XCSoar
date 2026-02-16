// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/ThermalAssistantWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Input/InputEvents.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

class GaugeThermalAssistantWindow : public ThermalAssistantWindow {
  bool dragging = false, pressed = false;

public:
  GaugeThermalAssistantWindow(ContainerWindow &parent,
                              PixelRect rc,
                              const ThermalAssistantLook &look,
                              WindowStyle style=WindowStyle()) noexcept
    :ThermalAssistantWindow(look, 5, true, true)
  {
    Create(parent, rc, style);
  }

private:
  void SetPressed(bool _pressed) noexcept {
    if (_pressed == pressed)
      return;

    pressed = _pressed;
    Invalidate();
  }

protected:
  void OnCancelMode() noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};

void
GaugeThermalAssistantWindow::OnCancelMode() noexcept
{
  if (dragging) {
    dragging = false;
    pressed = false;
    Invalidate();
    ReleaseCapture();
  }

  ThermalAssistantWindow::OnCancelMode();
}

bool
GaugeThermalAssistantWindow::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  if (!dragging) {
    dragging = true;
    SetCapture();

    pressed = true;
    Invalidate();
  }

  return true;
}

bool
GaugeThermalAssistantWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (dragging) {
    const bool was_pressed = pressed;

    dragging = false;
    pressed = false;
    Invalidate();

    ReleaseCapture();

    if (was_pressed)
      InputEvents::eventThermalAssistant("");

    return true;
  }

  return false;
}

bool
GaugeThermalAssistantWindow::OnMouseMove(PixelPoint p,
                                         [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  }

  return false;
}

void
GaugeThermalAssistantWindow::OnPaint(Canvas &canvas) noexcept
{
  ThermalAssistantWindow::OnPaint(canvas);

  if (pressed) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;

    canvas.SelectNullPen();
    canvas.Select(Brush(COLOR_YELLOW.WithAlpha(80)));

    DrawCircle(canvas);
#else
    canvas.InvertRectangle(GetClientRect());
#endif
  }
}

void
GaugeThermalAssistant::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  SetWindow(std::make_unique<GaugeThermalAssistantWindow>(parent, rc,
                                                          look, style));
}

void
GaugeThermalAssistant::Show(const PixelRect &rc) noexcept
{
  Update(blackboard.Basic().attitude, blackboard.Calculated());

  OverlappedWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GaugeThermalAssistant::Hide() noexcept
{
  blackboard.RemoveListener(*this);
  OverlappedWidget::Hide();
}

bool
GaugeThermalAssistant::SetFocus() noexcept
{
  return false;
}

void
GaugeThermalAssistant::OnCalculatedUpdate(const MoreData &basic,
                                          const DerivedInfo &calculated)
{
  Update(basic.attitude, calculated);
}

void
GaugeThermalAssistant::Update(const AttitudeState &attitude,
                              const DerivedInfo &calculated) noexcept
{
  ThermalAssistantWindow &window = (ThermalAssistantWindow &)GetWindow();
  window.Update(attitude, calculated);
}
