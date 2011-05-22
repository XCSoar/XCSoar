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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
//#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "VarioConfigPanel.hpp"

static WndForm* wf = NULL;


void
VarioConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  LoadFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                   XCSoarInterface::main_window.vario->ShowSpeedToFly);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                   XCSoarInterface::main_window.vario->ShowAvgText);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                   XCSoarInterface::main_window.vario->ShowMc);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                   XCSoarInterface::main_window.vario->ShowBugs);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                   XCSoarInterface::main_window.vario->ShowBallast);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                   XCSoarInterface::main_window.vario->ShowGross);

  LoadFormProperty(*wf, _T("prpAppAveNeedle"),
                   XCSoarInterface::main_window.vario->ShowAveNeedle);
}


bool
VarioConfigPanel::Save()
{
  bool changed = false;

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                              szProfileAppGaugeVarioSpeedToFly,
                              XCSoarInterface::main_window.vario->ShowSpeedToFly);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                              szProfileAppGaugeVarioAvgText,
                              XCSoarInterface::main_window.vario->ShowAvgText);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                              szProfileAppGaugeVarioMc,
                              XCSoarInterface::main_window.vario->ShowMc);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                              szProfileAppGaugeVarioBugs,
                              XCSoarInterface::main_window.vario->ShowBugs);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                              szProfileAppGaugeVarioBallast,
                              XCSoarInterface::main_window.vario->ShowBallast);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                              szProfileAppGaugeVarioGross,
                              XCSoarInterface::main_window.vario->ShowGross);

  changed |= SaveFormProperty(*wf, _T("prpAppAveNeedle"), szProfileAppAveNeedle,
                              XCSoarInterface::main_window.vario->ShowAveNeedle);

  return changed;
}
