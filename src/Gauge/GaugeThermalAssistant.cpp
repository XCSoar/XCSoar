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

#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/ThermalAssistantWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Input/InputEvents.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

class GaugeThermalAssistantWindow : public ThermalAssistantWindow {
  bool dragging, pressed;

public:
  GaugeThermalAssistantWindow(ContainerWindow &parent,
                              PixelRect rc,
                              const ThermalAssistantLook &look,
                              WindowStyle style=WindowStyle())
    :ThermalAssistantWindow(look, 5, true, true),
     dragging(false), pressed(false)
  {
    Create(parent, rc, style);
  }

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

void
GaugeThermalAssistantWindow::OnCancelMode()
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
GaugeThermalAssistantWindow::OnMouseDown(PixelPoint p)
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
GaugeThermalAssistantWindow::OnMouseUp(PixelPoint p)
{
  if (dragging) {
    const bool was_pressed = pressed;

    dragging = false;
    pressed = false;
    Invalidate();

    ReleaseCapture();

    if (was_pressed)
      InputEvents::eventThermalAssistant(_T(""));

    return true;
  }

  return false;
}

bool
GaugeThermalAssistantWindow::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (dragging) {
    SetPressed(IsInside(p));
    return true;
  }

  return false;
}

void
GaugeThermalAssistantWindow::OnPaint(Canvas &canvas)
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
GaugeThermalAssistant::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();

  GaugeThermalAssistantWindow *window =
    new GaugeThermalAssistantWindow(parent, rc, look, style);
  SetWindow(window);
}

void
GaugeThermalAssistant::Unprepare()
{
  DeleteWindow();
  OverlappedWidget::Unprepare();
}

void
GaugeThermalAssistant::Show(const PixelRect &rc)
{
  Update(blackboard.Basic().attitude, blackboard.Calculated());

  OverlappedWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GaugeThermalAssistant::Hide()
{
  blackboard.RemoveListener(*this);
  OverlappedWidget::Hide();
}

bool
GaugeThermalAssistant::SetFocus()
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
                              const DerivedInfo &calculated)
{
  ThermalAssistantWindow &window = (ThermalAssistantWindow &)GetWindow();
  window.Update(attitude, calculated);
}
