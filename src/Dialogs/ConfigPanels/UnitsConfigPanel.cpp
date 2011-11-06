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

#include "UnitsConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/Float.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Units/UnitsStore.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;
static unsigned SpeedUnits = 1; // default is knots
static unsigned TaskSpeedUnits = 2; // default is kph
static unsigned DistanceUnits = 2; // default is km
static unsigned LiftUnits = 0;
static unsigned AltitudeUnits = 0; //default ft
static unsigned TemperatureUnits = 0; //default is celcius
static bool loading = false;


static void
UpdateUnitFields(const UnitSetting &units)
{
  unsigned index;
  index = (units.speed_unit == unStatuteMilesPerHour) ? 0 :
          (units.speed_unit == unKnots) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsSpeed"), index);
  if (loading)
    SpeedUnits = index;

  index = (units.task_speed_unit == unStatuteMilesPerHour) ? 0 :
          (units.task_speed_unit == unKnots) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsTaskSpeed"), index);
  if (loading)
    TaskSpeedUnits = index;

  index = (units.distance_unit == unStatuteMiles) ? 0 :
          (units.distance_unit == unNauticalMiles) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsDistance"), index);
  if (loading)
    DistanceUnits = index;

  index = (units.altitude_unit == unFeet) ? 0 : 1;
  LoadFormProperty(*wf, _T("prpUnitsAltitude"), index);
  if (loading)
    AltitudeUnits = index;

  index = (units.temperature_unit == unGradFahrenheit) ? 1 : 0;
  LoadFormProperty(*wf, _T("prpUnitsTemperature"), index);
  if (loading)
    TemperatureUnits = index;

  index = (units.vertical_speed_unit == unKnots) ? 0 :
          (units.vertical_speed_unit == unFeetPerMinute) ? 2 : 1;
  LoadFormProperty(*wf, _T("prpUnitsLift"), index);
  if (loading)
    LiftUnits = index;
}


static void
SetUnitsTitle(const TCHAR* title)
{
  TCHAR caption[255];
  _tcscpy(caption,  _("Units"));
  _tcscat(caption, _T(": "));
  _tcscat(caption, title);
  ((WndFrame *)wf->FindByName(_T("lblUnitsSetting")))->SetCaption(caption);
}


static void
UpdateUnitsTitle()
{
  TCHAR title[255];
  if (Profile::Get(szProfileUnitsPresetName, title, 255))
    SetUnitsTitle(title);
}


void
UnitsConfigPanel::OnLoadPreset(WndButton &button)
{
  ComboList list;
  unsigned len = Units::Store::Count();
  for (unsigned i = 0; i < len; i++)
    list.Append(i, Units::Store::GetName(i));

  list.Sort();

  /* let the user select */

  int result = ComboPicker(XCSoarInterface::main_window, _("Unit Presets"), list, NULL);
  if (result >= 0) {
    const UnitSetting& units = Units::Store::Read(list[result].DataFieldIndex);
    UpdateUnitFields(units);

    Profile::Set(szProfileUnitsPresetName, list[result].StringValue);
    UpdateUnitsTitle();
  }
}


void
UnitsConfigPanel::OnFieldData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    if (!loading)
      Profile::Set(szProfileUnitsPresetName, _T("Custom"));
    UpdateUnitsTitle();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
SetLocalTime(int utc_offset)
{
  WndProperty* wp;
  TCHAR temp[20];
  int time(XCSoarInterface::Basic().time);
  Units::TimeToTextHHMMSigned(temp, TimeLocal(time, utc_offset));

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  assert(wp != NULL);

  wp->SetText(temp);
  wp->RefreshDisplay();
}


void
UnitsConfigPanel::OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch(Mode) {
  case DataField::daChange:
  {
    DataFieldFloat &df = *(DataFieldFloat *)Sender;
    int ival = iround(df.GetAsFixed() * 3600);
    SetLocalTime(ival);
    break;
  }
  case DataField::daSpecial:
    return;
  }
}


void
UnitsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  loading = true;

  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  assert(wp != NULL);
  DataFieldEnum *dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("mph"));
  dfe->addEnumText(_("knots"));
  dfe->addEnumText(_("km/h"));
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  const TCHAR *const units_lat_lon[] = {
    _T("DDMMSS"),
    _T("DDMMSS.ss"),
    _T("DDMM.mmm"),
    _T("DD.dddd"),
    NULL
  };

  dfe->addEnumTexts(units_lat_lon);
  dfe->Set(Units::GetCoordinateFormat());
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("mph"));
  dfe->addEnumText(_("knots"));
  dfe->addEnumText(_("km/h"));
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("sm"));
  dfe->addEnumText(_("nm"));
  dfe->addEnumText(_("km"));
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("foot"));
  dfe->addEnumText(_("meter"));
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTemperature"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("C"));
  dfe->addEnumText(_("F"));
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("knots"));
  dfe->addEnumText(_("m/s"));
  dfe->addEnumText(_("ft/min"));
  wp->RefreshDisplay();

  UpdateUnitFields(Units::current);

  int utc_offset = XCSoarInterface::SettingsComputer().utc_offset;
  LoadFormProperty(*wf, _T("prpUTCOffset"),
                   fixed(iround(fixed(utc_offset) / 1800)) / 2);
#ifdef WIN32
  if (is_embedded() && !is_altair())
    ((WndProperty*)wf->FindByName(_T("prpUTCOffset")))->set_enabled(false);
#endif
  SetLocalTime(utc_offset);

  loading = false;
}


bool
UnitsConfigPanel::Save()
{
  bool changed = false;

  /* the Units settings affect how other form values are read and translated
   * so changes to Units settings should be processed after all other form settings
   */
  int tmp = GetFormValueInteger(*wf, _T("prpUnitsSpeed"));
  if ((int)SpeedUnits != tmp) {
    SpeedUnits = tmp;
    Profile::Set(szProfileSpeedUnitsValue, SpeedUnits);
    changed = true;

    switch (SpeedUnits) {
    case 0:
      Units::SetUserSpeedUnit(unStatuteMilesPerHour);
      Units::SetUserWindSpeedUnit(unStatuteMilesPerHour);
      break;
    case 1:
      Units::SetUserSpeedUnit(unKnots);
      Units::SetUserWindSpeedUnit(unKnots);
      break;
    case 2:
    default:
      Units::SetUserSpeedUnit(unKiloMeterPerHour);
      Units::SetUserWindSpeedUnit(unKiloMeterPerHour);
      break;
    }
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsLatLon"));
  if ((int)Units::GetCoordinateFormat() != tmp) {
    Units::SetCoordinateFormat((CoordinateFormats)tmp);
    Profile::Set(szProfileLatLonUnits, Units::GetCoordinateFormat());
    changed = true;
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsTaskSpeed"));
  if ((int)TaskSpeedUnits != tmp) {
    TaskSpeedUnits = tmp;
    Profile::Set(szProfileTaskSpeedUnitsValue, TaskSpeedUnits);
    changed = true;

    switch (TaskSpeedUnits) {
    case 0:
      Units::SetUserTaskSpeedUnit(unStatuteMilesPerHour);
      break;
    case 1:
      Units::SetUserTaskSpeedUnit(unKnots);
      break;
    case 2:
    default:
      Units::SetUserTaskSpeedUnit(unKiloMeterPerHour);
      break;
    }
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsDistance"));
  if ((int)DistanceUnits != tmp) {
    DistanceUnits = tmp;
    Profile::Set(szProfileDistanceUnitsValue, DistanceUnits);
    changed = true;

    switch (DistanceUnits) {
    case 0:
      Units::SetUserDistanceUnit(unStatuteMiles);
      break;
    case 1:
      Units::SetUserDistanceUnit(unNauticalMiles);
      break;
    case 2:
    default:
      Units::SetUserDistanceUnit(unKiloMeter);
      break;
    }
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsLift"));
  if ((int)LiftUnits != tmp) {
    LiftUnits = tmp;
    Profile::Set(szProfileLiftUnitsValue, LiftUnits);
    changed = true;

    switch (LiftUnits) {
    case 0:
      Units::SetUserVerticalSpeedUnit(unKnots);
      break;
    case 1:
    default:
      Units::SetUserVerticalSpeedUnit(unMeterPerSecond);
      break;
    case 2:
      Units::SetUserVerticalSpeedUnit(unFeetPerMinute);
      break;
    }
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsAltitude"));
  if ((int)AltitudeUnits != tmp) {
    AltitudeUnits = tmp;
    Profile::Set(szProfileAltitudeUnitsValue, AltitudeUnits);
    changed = true;

    switch (AltitudeUnits) {
    case 0:
      Units::SetUserAltitudeUnit(unFeet);
      break;
    case 1:
    default:
      Units::SetUserAltitudeUnit(unMeter);
      break;
    }
  }

  tmp = GetFormValueInteger(*wf, _T("prpUnitsTemperature"));
  if ((int)TemperatureUnits != tmp) {
    TemperatureUnits = tmp;
    Profile::Set(szProfileTemperatureUnitsValue, TemperatureUnits);
    changed = true;

    switch (TemperatureUnits) {
    case 0:
      Units::SetUserTemperatureUnit(unGradCelcius);
      break;
    case 1:
    default:
      Units::SetUserTemperatureUnit(unGradFahrenheit);
      break;
    }
  }

  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  int ival = iround(GetFormValueFixed(*wf, _T("prpUTCOffset")) * 3600);
  if (settings_computer.utc_offset != ival) {
    settings_computer.utc_offset = ival;

    // have to do this because registry variables can't be negative!
    if (ival < 0)
      ival += 24 * 3600;

    Profile::Set(szProfileUTCOffset, ival);
    changed = true;
  }

  return changed;
}
