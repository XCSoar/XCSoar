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

#include "Gauge/GaugeFLARM.hpp"
#include "FlarmTrafficWindow.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "NMEA/MoreData.hpp"
#include "ComputerSettings.hpp"
#include "UIActions.hpp"

/**
 * Widget to display a FLARM gauge
 */
class SmallTrafficWindow : public FlarmTrafficWindow {
public:
  SmallTrafficWindow(ContainerWindow &parent, const PixelRect &rc,
                     const FlarmTrafficLook &look,
                     const WindowStyle style=WindowStyle());

  void Update(const NMEAInfo &gps_info, const TeamCodeSettings &settings);

protected:
  bool OnMouseDown(PixelScalar x, PixelScalar y);
};

/**
 * Constructor of the SmallTrafficWindow class
 * @param parent Parent window
 * @param left Left edge of window pixel location
 * @param top Top edge of window pixel location
 * @param width Width of window (pixels)
 * @param height Height of window (pixels)
 */
SmallTrafficWindow::SmallTrafficWindow(ContainerWindow &parent,
                                       const PixelRect &rc,
                                       const FlarmTrafficLook &look,
                                       const WindowStyle style)
  :FlarmTrafficWindow(look, 1, 1, true)
{
  Create(parent, rc, style);
}

void
SmallTrafficWindow::Update(const NMEAInfo &gps_info,
                           const TeamCodeSettings &settings)
{
  FlarmTrafficWindow::Update(gps_info.track, gps_info.flarm.traffic, settings);
}

/**
 * This function is called when the mouse is pressed on the FLARM gauge and
 * opens the FLARM Traffic dialog
 * @param x x-Coordinate of the click
 * @param y y-Coordinate of the click
 */
bool
SmallTrafficWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{
  UIActions::ShowTrafficRadar();
  return true;
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
  SmallTrafficWindow *window =
    (SmallTrafficWindow *)OverlappedWidget::GetWindow();
  delete window;

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
  SmallTrafficWindow *window =
    (SmallTrafficWindow *)GetWindow();

  window->Update(basic, blackboard.GetComputerSettings().team_code);
}
