/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Blackboard/LiveBlackboard.hpp"
#include "Input/InputEvents.hpp"

class GaugeThermalAssistantWindow : public ThermalAssistantWindow {
public:
  GaugeThermalAssistantWindow(ContainerWindow &parent,
                              PixelRect rc,
                              const ThermalAssistantLook &look,
                              WindowStyle style=WindowStyle())
    :ThermalAssistantWindow(look, 5, true)
  {
    Create(parent, rc, style);
  }

protected:
  bool OnMouseDown(PixelScalar x, PixelScalar y);
};

/**
 * This function is called when the mouse is pressed on the FLARM gauge and
 * opens the FLARM Traffic dialog
 * @param x x-Coordinate of the click
 * @param y x-Coordinate of the click
 * @return
 */
bool
GaugeThermalAssistantWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{
  InputEvents::eventThermalAssistant(_T(""));
  return true;
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
  GaugeThermalAssistantWindow *window =
    (GaugeThermalAssistantWindow *)OverlappedWidget::GetWindow();
  delete window;

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
  ThermalAssistantWindow *window =
    (ThermalAssistantWindow *)GetWindow();

  window->Update(attitude, calculated);
}
