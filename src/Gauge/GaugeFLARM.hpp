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

#ifndef GAUGE_FLARM_HPP
#define GAUGE_FLARM_HPP

#include "FlarmTrafficWindow.hpp"

struct NMEAInfo;
struct SETTINGS_TEAMCODE;
class ContainerWindow;

/**
 * Widget to display a FLARM gauge
 */
class GaugeFLARM : public FlarmTrafficWindow {
public:
  bool ForceVisible, Suppress;

public:
  GaugeFLARM(ContainerWindow &parent,
             int left, int top, unsigned width, unsigned height,
             const FlarmTrafficLook &look,
             const WindowStyle style=WindowStyle());

  void Update(bool enable, const NMEAInfo &gps_info,
              const SETTINGS_TEAMCODE &settings);

protected:
  bool on_mouse_down(int x, int y);
};

#endif
