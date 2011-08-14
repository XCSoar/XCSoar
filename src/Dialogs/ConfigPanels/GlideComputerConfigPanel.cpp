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
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "GlideComputerConfigPanel.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;


void
GlideComputerConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;
  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Manual"));
    dfe->addEnumText(_("Circling"));
    dfe->addEnumText(_("ZigZag"));
    dfe->addEnumText(_("Both"));
    dfe->Set(settings_computer.AutoWindMode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpExternalWind"), settings_computer.ExternalWind);

  wp = (WndProperty*)wf->FindByName(_T("prpAutoMcMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Final glide"));
    dfe->addEnumText(_("Trending average climb"));
    dfe->addEnumText(_("Both"));
    dfe->Set((int)settings_computer.auto_mc_mode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpBlockSTF"),
                   settings_computer.EnableBlockSTF);

  LoadFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                   settings_computer.EnableNavBaroAltitude);

  LoadFormProperty(*wf, _T("prpEnableExternalTriggerCruise"),
                   settings_computer.EnableExternalTriggerCruise);

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("15 s"));
    dfe->addEnumText(_T("30 s"));
    dfe->addEnumText(_T("60 s"));
    dfe->addEnumText(_T("90 s"));
    dfe->addEnumText(_T("2 min"));
    dfe->addEnumText(_T("3 min"));
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(settings_computer.AverEffTime);
    wp->RefreshDisplay();
  }
}


bool
GlideComputerConfigPanel::Save(bool &requirerestart)
{
  bool changed = false;
  WndProperty *wp;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(*wf, _T("prpAutoWind"), szProfileAutoWind,
                              settings_computer.AutoWindMode);

  changed |= SaveFormProperty(*wf, _T("prpExternalWind"),
                              szProfileExternalWind,
                              settings_computer.ExternalWind);

  int auto_mc_mode = (int)settings_computer.auto_mc_mode;
  changed |= SaveFormProperty(*wf, _T("prpAutoMcMode"), szProfileAutoMcMode,
                              auto_mc_mode);
  settings_computer.auto_mc_mode = (TaskBehaviour::AutoMCMode_t)auto_mc_mode;

  changed |= SaveFormProperty(*wf, _T("prpBlockSTF"),
                              szProfileBlockSTF,
                              settings_computer.EnableBlockSTF);

  changed |= SaveFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                              szProfileEnableNavBaroAltitude,
                              settings_computer.EnableNavBaroAltitude);

  changed |= SaveFormProperty(*wf, _T("prpEnableExternalTriggerCruise"),
                              szProfileEnableExternalTriggerCruise,
                              settings_computer.EnableExternalTriggerCruise);

  wp = (WndProperty*)wf->FindByName(_T("prpAverEffTime")); // VENTA6
  if (wp) {
    if (settings_computer.AverEffTime != wp->GetDataField()->GetAsInteger()) {
      settings_computer.AverEffTime = wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAverEffTime,
                   settings_computer.AverEffTime);
      changed = true;
      requirerestart = true;
    }
  }

  return changed;
}
