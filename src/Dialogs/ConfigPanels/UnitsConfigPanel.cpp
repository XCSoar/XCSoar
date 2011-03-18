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
#include "UnitsStore.hpp"
#include "UnitsFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Language.hpp"

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
  index = (units.SpeedUnit == unStatuteMilesPerHour) ? 0 :
          (units.SpeedUnit == unKnots) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsSpeed"), index);
  if (loading)
    SpeedUnits = index;

  index = (units.TaskSpeedUnit == unStatuteMilesPerHour) ? 0 :
          (units.TaskSpeedUnit == unKnots) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsTaskSpeed"), index);
  if (loading)
    TaskSpeedUnits = index;

  index = (units.DistanceUnit == unStatuteMiles) ? 0 :
          (units.DistanceUnit == unNauticalMiles) ? 1 : 2;
  LoadFormProperty(*wf, _T("prpUnitsDistance"), index);
  if (loading)
    DistanceUnits = index;

  index = (units.AltitudeUnit == unFeet) ? 0 : 1;
  LoadFormProperty(*wf, _T("prpUnitsAltitude"), index);
  if (loading)
    AltitudeUnits = index;

  index = (units.TemperatureUnit == unGradFahrenheit) ? 1 : 0;
  LoadFormProperty(*wf, _T("prpUnitsTemperature"), index);
  if (loading)
    TemperatureUnits = index;

  index = (units.VerticalSpeedUnit == unKnots) ? 0 :
          (units.VerticalSpeedUnit == unFeetPerMinute) ? 2 : 1;
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

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}


static void
SetLocalTime(void)
{
  WndProperty* wp;
  TCHAR temp[20];
  Units::TimeToTextHHMMSigned(temp, TimeLocal(XCSoarInterface::Basic().Time));

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
    if (XCSoarInterface::SettingsComputer().UTCOffset != ival)
      XCSoarInterface::SetSettingsComputer().UTCOffset = ival;

    SetLocalTime();
    break;
  }
  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}


void
UnitsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  WndProperty *wp;
  loading = true;

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("mph"));
    dfe->addEnumText(_("knots"));
    dfe->addEnumText(_("km/h"));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    DataFieldEnum* dfe;
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("mph"));
    dfe->addEnumText(_("knots"));
    dfe->addEnumText(_("km/h"));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("sm"));
    dfe->addEnumText(_("nm"));
    dfe->addEnumText(_("km"));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("foot"));
    dfe->addEnumText(_("meter"));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTemperature"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("C"));
    dfe->addEnumText(_("F"));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("knots"));
    dfe->addEnumText(_("m/s"));
    dfe->addEnumText(_("ft/min"));
    wp->RefreshDisplay();
  }

  UpdateUnitFields(Units::Current);

  LoadFormProperty(*wf, _T("prpUTCOffset"), fixed(iround(
         fixed(XCSoarInterface::SettingsComputer().UTCOffset) / 1800)) / 2);
#ifdef WIN32
  if (is_embedded() && !is_altair())
    ((WndProperty*)wf->FindByName(_T("prpUTCOffset")))->set_enabled(false);
#endif
  SetLocalTime();

  loading = false;
}


bool
UnitsConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;

  /* the Units settings affect how other form values are read and translated
   * so changes to Units settings should be processed after all other form settings
   */
  wp = (WndProperty*)wf->FindByName(_T("prpUnitsSpeed"));
  if (wp) {
    if ((int)SpeedUnits != wp->GetDataField()->GetAsInteger()) {
      SpeedUnits = wp->GetDataField()->GetAsInteger();
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLatLon"));
  if (wp) {
    if ((int)Units::GetCoordinateFormat() != wp->GetDataField()->GetAsInteger()) {
      Units::SetCoordinateFormat(
          (CoordinateFormats_t)wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileLatLonUnits, Units::GetCoordinateFormat());
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTaskSpeed"));
  if (wp) {
    if ((int)TaskSpeedUnits != wp->GetDataField()->GetAsInteger()) {
      TaskSpeedUnits = wp->GetDataField()->GetAsInteger();
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsDistance"));
  if (wp) {
    if ((int)DistanceUnits != wp->GetDataField()->GetAsInteger()) {
      DistanceUnits = wp->GetDataField()->GetAsInteger();
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsLift"));
  if (wp) {
    if ((int)LiftUnits != wp->GetDataField()->GetAsInteger()) {
      LiftUnits = wp->GetDataField()->GetAsInteger();
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsAltitude"));
  if (wp) {
    if ((int)AltitudeUnits != wp->GetDataField()->GetAsInteger()) {
      AltitudeUnits = wp->GetDataField()->GetAsInteger();
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
  }

  wp = (WndProperty*)wf->FindByName(_T("prpUnitsTemperature"));
  if (wp) {
    if ((int)TemperatureUnits != wp->GetDataField()->GetAsInteger()) {
      TemperatureUnits = wp->GetDataField()->GetAsInteger();
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
  }

  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  wp = (WndProperty*)wf->FindByName(_T("prpUTCOffset"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    int ival = iround(df.GetAsFixed() * 3600);
    if (settings_computer.UTCOffset != ival) {
      settings_computer.UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      if (ival < 0)
        ival += 24 * 3600;

      Profile::Set(szProfileUTCOffset, ival);
      changed = true;
    }
  }

  return changed;
}
