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

#include "AltitudeSimulator.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/OffsetButtonsWidget.hpp"
#include "Components.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"

class AltitudeSimulatorOffsetButtons final : public OffsetButtonsWidget {
public:
  template<typename... Args>
  AltitudeSimulatorOffsetButtons(Args&&... args):OffsetButtonsWidget(args...) {}

protected:
  /* virtual methods from OffsetButtonsWidget */
  virtual void OnOffset(double offset) override;
};

void
AltitudeSimulatorOffsetButtons::OnOffset(const double step)
{
  if (!is_simulator())
    return;

  const NMEAInfo &basic = CommonInterface::Basic();

  device_blackboard->SetAltitude(basic.gps_altitude +
                                 Units::ToSysAltitude(step));
}

Widget *
LoadAltitudeSimulatorPanel(unsigned id)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.gps.simulator)
    return nullptr;

  return new AltitudeSimulatorOffsetButtons(UIGlobals::GetDialogLook().button,
                                            _T("%+.0f"),
                                            10, 100);
}
