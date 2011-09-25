/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Dialogs/Dialogs.h"

/**
 * Constructor of the GaugeFLARM class
 * @param parent Parent window
 * @param left Left edge of window pixel location
 * @param top Top edge of window pixel location
 * @param width Width of window (pixels)
 * @param height Height of window (pixels)
 */
GaugeThermalAssistant::GaugeThermalAssistant(ContainerWindow &parent,
                                             PixelScalar left, PixelScalar top,
                                             UPixelScalar width,
                                             UPixelScalar height,
                                             WindowStyle style)
  :ThermalAssistantWindow(5, true)
{
  set(parent, left, top, width, height, style);
}

void
GaugeThermalAssistant::Update(const bool enabled, const Angle direction,
                              const DerivedInfo &derived)
{
  if (enabled && derived.circling) {
    ThermalAssistantWindow::Update(direction, derived);
    show();
  } else {
    hide();
  }
}

/**
 * This function is called when the mouse is pressed on the FLARM gauge and
 * opens the FLARM Traffic dialog
 * @param x x-Coordinate of the click
 * @param y x-Coordinate of the click
 * @return
 */
bool
GaugeThermalAssistant::on_mouse_down(PixelScalar x, PixelScalar y)
{
  dlgThermalAssistantShowModal();
  return true;
}
