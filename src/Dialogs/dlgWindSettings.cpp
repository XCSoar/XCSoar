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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "MapSettings.hpp"
#include "ComputerSettings.hpp"
#include "Units/Units.hpp"
#include "Profile/Profile.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/Boolean.hpp"
#include "MainWindow.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

struct WindDialogSettings
{
  SpeedVector wind;

  uint8_t auto_wind_mode;
  bool trail_drift_enabled;
};

static WndForm *wf = NULL;
static bool external_wind;
static WindDialogSettings settings;

static void
OnCancel(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}

static void
OnOK(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
SetFieldValues(const WindDialogSettings &settings)
{
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  assert(wp != NULL);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.Set(Units::ToUserWindSpeed(settings.wind.norm));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
  assert(wp != NULL);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.Set(settings.wind.bearing.Degrees());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  assert(wp != NULL);
  if (!external_wind) {
    DataFieldEnum *dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->Set(settings.auto_wind_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTrailDrift"));
  assert(wp != NULL);
  {
    DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
    df.Set(settings.trail_drift_enabled);
    wp->RefreshDisplay();
  }
}

static void
GetFieldValues(WindDialogSettings &settings)
{
  if (!external_wind) {
    WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
    assert(wp != NULL);
    {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      settings.wind.norm = Units::ToSysWindSpeed(df.GetAsFixed());
    }

    wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
    assert(wp != NULL);
    {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      settings.wind.bearing = Angle::Degrees(df.GetAsFixed());
    }

    SaveFormProperty(*wf, _T("prpAutoWind"), settings.auto_wind_mode);
  }

  SaveFormProperty(*wf, _T("prpTrailDrift"), settings.trail_drift_enabled);
}

static void
ApplySettings(const WindDialogSettings &settings)
{
  if (!external_wind) {
    XCSoarInterface::SetComputerSettings().manual_wind = settings.wind;
    XCSoarInterface::SetComputerSettings().manual_wind_available.Update(
        XCSoarInterface::Basic().clock);

    XCSoarInterface::SetComputerSettings().auto_wind_mode =
        settings.auto_wind_mode;
  }

  XCSoarInterface::SetMapSettings().trail_drift_enabled =
      settings.trail_drift_enabled;

  ActionInterface::SendMapSettings();
}

static void
SaveSettings(const WindDialogSettings &settings)
{
  if (!external_wind)
    Profile::Set(szProfileAutoWind, settings.auto_wind_mode);

  Profile::Set(szProfileTrailDrift, settings.trail_drift_enabled);
}

static void
OnDataAccess(gcc_unused DataField *sender, DataField::DataAccessKind_t mode)
{
  if (mode != DataField::daChange)
    return;

  // Read field values
  WindDialogSettings settings;
  GetFieldValues(settings);

  // Use field values
  ApplySettings(settings);
}

static void
InitFields()
{
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  assert(wp != NULL);
  wp->set_enabled(!external_wind);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetDataAccessCallback(OnDataAccess);
    df.SetMax(Units::ToUserWindSpeed(Units::ToSysUnit(fixed(200), unKiloMeterPerHour)));
    df.SetUnits(Units::GetSpeedName());
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
  assert(wp != NULL);
  wp->set_enabled(!external_wind);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetDataAccessCallback(OnDataAccess);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  assert(wp != NULL);
  if (external_wind) {
    wp->set_enabled(false);
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_("External"));
    df.Set(0);
  } else {
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->SetDataAccessCallback(OnDataAccess);
    dfe->addEnumText(_("Manual"));
    dfe->addEnumText(_("Circling"));
    dfe->addEnumText(_("ZigZag"));
    dfe->addEnumText(_("Both"));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTrailDrift"));
  assert(wp != NULL);
  {
    DataField &df = *(DataField *)wp->GetDataField();
    df.SetDataAccessCallback(OnDataAccess);
  }
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnOK),
  DeclareCallBackEntry(OnCancel),
  DeclareCallBackEntry(NULL)
};

void
dlgWindSettingsShowModal(void)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
		                  _T("IDR_XML_WINDSETTINGS"));
  assert(wf != NULL);

  external_wind = XCSoarInterface::Basic().external_wind_available &&
                  XCSoarInterface::GetComputerSettings().use_external_wind,

  // Save current settings
  settings.wind = CommonInterface::Calculated().GetWindOrZero();
  settings.auto_wind_mode = XCSoarInterface::GetComputerSettings().auto_wind_mode;
  settings.trail_drift_enabled = XCSoarInterface::GetMapSettings().trail_drift_enabled;

  // Initialize field values
  InitFields();
  // Set fields according to settings
  SetFieldValues(settings);

  if (wf->ShowModal() == mrOK) {
    // Read current settings
    GetFieldValues(settings);
    // Apply current settings
    ApplySettings(settings);
    // Save current settings to the profile
    SaveSettings(settings);
  } else {
    // Re-apply original setting
    ApplySettings(settings);
  }

  delete wf;
}
