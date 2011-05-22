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
#include "GaugesConfigPanel.hpp"

static WndForm* wf = NULL;


void
GaugesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  LoadFormProperty(*wf, _T("prpEnableFLARMGauge"),
                   XCSoarInterface::SettingsMap().EnableFLARMGauge);

  LoadFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                   XCSoarInterface::SettingsMap().AutoCloseFlarmDialog);

  LoadFormProperty(*wf, _T("prpEnableTAGauge"),
                   XCSoarInterface::SettingsMap().EnableTAGauge);

  LoadFormProperty(*wf, _T("prpEnableThermalProfile"),
                   XCSoarInterface::SettingsMap().EnableThermalProfile);
}


bool
GaugesConfigPanel::Save()
{
  bool changed = false;

  changed |= SaveFormProperty(*wf, _T("prpEnableFLARMGauge"),
                              szProfileEnableFLARMGauge,
                              XCSoarInterface::SetSettingsMap().EnableFLARMGauge);

  changed |= SaveFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                              szProfileAutoCloseFlarmDialog,
                              XCSoarInterface::SetSettingsMap().AutoCloseFlarmDialog);

  changed |= SaveFormProperty(*wf, _T("prpEnableTAGauge"),
                              szProfileEnableTAGauge,
                              XCSoarInterface::SetSettingsMap().EnableTAGauge);

  changed |= SaveFormProperty(*wf, _T("prpEnableThermalProfile"),
                              szProfileEnableThermalProfile,
                              XCSoarInterface::SetSettingsMap().EnableThermalProfile);

  return changed;
}
