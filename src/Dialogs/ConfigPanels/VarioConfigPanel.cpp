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

#include "VarioConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Interface.hpp"

static WndForm* wf = NULL;


void
VarioConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const VarioSettings &settings = CommonInterface::GetUISettings().vario;

  LoadFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                   settings.ShowSpeedToFly);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                   settings.ShowAvgText);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                   settings.ShowMc);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                   settings.ShowBugs);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                   settings.ShowBallast);

  LoadFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                   settings.ShowGross);

  LoadFormProperty(*wf, _T("prpAppAveNeedle"),
                   settings.ShowAveNeedle);
}


bool
VarioConfigPanel::Save()
{
  bool changed = false;

  VarioSettings &settings = CommonInterface::SetUISettings().vario;

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioSpeedToFly"),
                              szProfileAppGaugeVarioSpeedToFly,
                              settings.ShowSpeedToFly);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioAvgText"),
                              szProfileAppGaugeVarioAvgText,
                              settings.ShowAvgText);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioMc"),
                              szProfileAppGaugeVarioMc,
                              settings.ShowMc);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBugs"),
                              szProfileAppGaugeVarioBugs,
                              settings.ShowBugs);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioBallast"),
                              szProfileAppGaugeVarioBallast,
                              settings.ShowBallast);

  changed |= SaveFormProperty(*wf, _T("prpAppGaugeVarioGross"),
                              szProfileAppGaugeVarioGross,
                              settings.ShowGross);

  changed |= SaveFormProperty(*wf, _T("prpAppAveNeedle"), szProfileAppAveNeedle,
                              settings.ShowAveNeedle);

  return changed;
}
