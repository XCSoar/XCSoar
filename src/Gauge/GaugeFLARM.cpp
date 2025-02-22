// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GaugeFLARM.hpp"
#include "ui/canvas/Canvas.hpp"
#include "FlarmTrafficWindow.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Computer/Settings.hpp"
#include "PageActions.hpp"
#include "Interface.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

class SmallTrafficWindow : public FlarmTrafficWindow {
  bool dragging = false, pressed = false;

public:
  SmallTrafficWindow(ContainerWindow &parent, const PixelRect &rc,
                     const FlarmTrafficLook &look,
                     const WindowStyle style=WindowStyle()) noexcept;

  void Update(const NMEAInfo &gps_info,
              const TeamCodeSettings &settings) noexcept;

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

SmallTrafficWindow::SmallTrafficWindow(ContainerWindow &parent,
                                       const PixelRect &rc,
                                       const FlarmTrafficLook &look,
                                       const WindowStyle style) noexcept
  :FlarmTrafficWindow(look, 1, 1, true)
{
  Create(parent, rc, style);
}

void
SmallTrafficWindow::Update(const NMEAInfo &gps_info,
                           const TeamCodeSettings &settings) noexcept
{
  FlarmTrafficWindow::Update(gps_info.track, gps_info.flarm.traffic, settings);
}

void
SmallTrafficWindow::OnCancelMode() noexcept
{
  if (dragging) {
    dragging = false;
    pressed = false;
    Invalidate();
    ReleaseCapture();
  }

  FlarmTrafficWindow::OnCancelMode();
}

bool
SmallTrafficWindow::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
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
SmallTrafficWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (dragging) {
    const bool was_pressed = pressed;

    dragging = false;
    pressed = false;
    Invalidate();

    ReleaseCapture();

    if (was_pressed) {
      TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
      settings.radar_zoom = 4;
      PageActions::ShowTrafficRadar();
    }

    return true;
  }

  return false;
}

bool
SmallTrafficWindow::OnMouseMove(PixelPoint p,
                                [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  }

  return false;
}

void
SmallTrafficWindow::OnPaint(Canvas &canvas) noexcept
{
  FlarmTrafficWindow::OnPaint(canvas);

  if (pressed) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.Clear(COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(GetClientRect());
#endif
  }
}

void
GaugeFLARM::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  SetWindow(std::make_unique<SmallTrafficWindow>(parent, rc, look, style));
}

void
GaugeFLARM::Show(const PixelRect &rc) noexcept
{
  Update(blackboard.Basic());

  OverlappedWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GaugeFLARM::Hide() noexcept
{
  blackboard.RemoveListener(*this);
  OverlappedWidget::Hide();
}

void
GaugeFLARM::OnGPSUpdate(const MoreData &basic)
{
  Update(basic);
}

void
GaugeFLARM::Update(const NMEAInfo &basic) noexcept
{
  SmallTrafficWindow &window = (SmallTrafficWindow &)GetWindow();
  window.Update(basic, blackboard.GetComputerSettings().team_code);
}
