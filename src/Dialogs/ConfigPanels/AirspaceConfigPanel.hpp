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

#ifndef AIRSPACECONFIGPANEL_HPP
#define AIRSPACECONFIGPANEL_HPP

#include "DataField/Base.hpp"

struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
class WndForm;
class WndButton;

namespace AirspaceConfigPanel
{
  void Init(WndForm *wf,
            const AirspaceComputerSettings &computer,
            const AirspaceRendererSettings &renderer);
  bool Save(bool &requirerestart,
            AirspaceComputerSettings &computer,
            AirspaceRendererSettings &renderer);

  void OnAirspaceColoursClicked(gcc_unused WndButton &button);
  void OnAirspaceModeClicked(gcc_unused WndButton &button);

  void OnAirspaceDisplay(DataField *Sender, DataField::DataAccessKind_t Mode);
  void OnAirspaceWarning(DataField *Sender, DataField::DataAccessKind_t Mode);
};

#endif /* AIRSPACECONFIGPANEL_HPP */
