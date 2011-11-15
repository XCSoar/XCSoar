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

#include "AltitudeSimulator.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Simulator.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/XMLWidget.hpp"

class WndButton;

class AltitudeSimulatorPanel : public XMLWidget {
public:
  void Refresh();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
};

static void
ChangeAltitude(const fixed step)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  device_blackboard->SetAltitude(basic.gps_altitude +
                                 (fixed)Units::ToSysAltitude(step));
}

static void
PnlSimulatorOnPlusBig(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(+100));
}

static void
PnlSimulatorOnPlusSmall(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(+10));
}

static void
PnlSimulatorOnMinusSmall(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(-10));
}

static void
PnlSimulatorOnMinusBig(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(-100));
}

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PnlSimulatorOnPlusBig),
  DeclareCallBackEntry(PnlSimulatorOnPlusSmall),
  DeclareCallBackEntry(PnlSimulatorOnMinusSmall),
  DeclareCallBackEntry(PnlSimulatorOnMinusBig),
  DeclareCallBackEntry(NULL)
};

void
AltitudeSimulatorPanel::Prepare(ContainerWindow &parent,
                                const PixelRect &rc)
{
  LoadWindow(CallBackTable, parent, _T("IDR_XML_INFOBOXALTITUDESIMULATOR"));
}

Widget *
LoadAltitudeSimulatorPanel(unsigned id)
{
  return new AltitudeSimulatorPanel();
}
