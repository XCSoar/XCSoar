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

#include "Gauge/GaugeFLARM.hpp"
#include "Screen/Canvas.hpp"
#include "FlarmTrafficWindow.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Computer/Settings.hpp"
#include "PageActions.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

class SmallTrafficWindow : public FlarmTrafficWindow {
  bool dragging, pressed;

public:
  SmallTrafficWindow(ContainerWindow &parent, const PixelRect &rc,
                     const FlarmTrafficLook &look,
                     const WindowStyle style=WindowStyle());

  void Update(const NMEAInfo &gps_info, const TeamCodeSettings &settings);

private:
  void SetPressed(bool _pressed) {
    if (_pressed == pressed)
      return;

    pressed = _pressed;
    Invalidate();
  }

protected:
  virtual void OnCancelMode() override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  virtual void OnPaint(Canvas &canvas) override;
};

SmallTrafficWindow::SmallTrafficWindow(ContainerWindow &parent,
                                       const PixelRect &rc,
                                       const FlarmTrafficLook &look,
                                       const WindowStyle style)
  :FlarmTrafficWindow(look, 1, 1, true),
   dragging(false), pressed(false)
{
  Create(parent, rc, style);
}

void
SmallTrafficWindow::Update(const NMEAInfo &gps_info,
                           const TeamCodeSettings &settings)
{
  FlarmTrafficWindow::Update(gps_info.track, gps_info.flarm.traffic, settings);
}

void
SmallTrafficWindow::OnCancelMode()
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
SmallTrafficWindow::OnMouseDown(PixelPoint p)
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
SmallTrafficWindow::OnMouseUp(PixelPoint p)
{
  if (dragging) {
    const bool was_pressed = pressed;

    dragging = false;
    pressed = false;
    Invalidate();

    ReleaseCapture();

    if (was_pressed)
      PageActions::ShowTrafficRadar();

    return true;
  }

  return false;
}

bool
SmallTrafficWindow::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  }

  return false;
}

void
SmallTrafficWindow::OnPaint(Canvas &canvas)
{
  FlarmTrafficWindow::OnPaint(canvas);

  if (pressed) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(0, 0, canvas.GetWidth(), canvas.GetHeight(),
                               COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(GetClientRect());
#endif
  }
}

void
GaugeFLARM::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();

  SetWindow(new SmallTrafficWindow(parent, rc, look, style));
}

void
GaugeFLARM::Unprepare()
{
  DeleteWindow();
  OverlappedWidget::Unprepare();
}

void
GaugeFLARM::Show(const PixelRect &rc)
{
  Update(blackboard.Basic());

  OverlappedWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GaugeFLARM::Hide()
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
GaugeFLARM::Update(const NMEAInfo &basic)
{
  SmallTrafficWindow &window = (SmallTrafficWindow &)GetWindow();
  window.Update(basic, blackboard.GetComputerSettings().team_code);
}
