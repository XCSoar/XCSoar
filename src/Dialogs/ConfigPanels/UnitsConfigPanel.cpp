/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "UIGlobals.hpp"
#include "DataField/Enum.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Units/Units.hpp"
#include "Units/UnitsStore.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "DataField/Base.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  UnitsPreset,
  spacer_1,
  UnitsSpeed,
  UnitsDistance,
  UnitsLift,
  UnitsAltitude,
  UnitsTemperature,
  UnitsTaskSpeed,
  UnitsPressure,
  spacer_2,
  UnitsLatLon
};

static const TCHAR *custom_preset_label = N_("Custom");

class UnitsConfigPanel : public RowFormWidget {
public:
  UnitsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  void UpdateUnitFields(const UnitSetting &units);
  void PresetCheck(DataField::DataAccessKind_t mode);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static UnitsConfigPanel *instance;

void
UnitsConfigPanel::UpdateUnitFields(const UnitSetting &units)
{
  LoadValueEnum(UnitsSpeed, units.speed_unit);
  LoadValueEnum(UnitsDistance, units.distance_unit);
  LoadValueEnum(UnitsLift, units.vertical_speed_unit);
  LoadValueEnum(UnitsAltitude, units.altitude_unit);
  LoadValueEnum(UnitsTemperature, units.temperature_unit);
  LoadValueEnum(UnitsTaskSpeed, units.task_speed_unit);
  LoadValueEnum(UnitsPressure, units.pressure_unit);

  // Ignore the coord.format for the preset selection.
}

static void
OnUnitsPreset(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  int result = instance->GetValueInteger(UnitsPreset);
  if (result > 0) {
    // First selection means not to load any preset.
    const UnitSetting& units = Units::Store::Read(result-1);
    instance->UpdateUnitFields(units);
  }
}

void
UnitsConfigPanel::PresetCheck(DataField::DataAccessKind_t mode)
{
  switch (mode) {
  case DataField::daChange:
  {
    UnitSetting current_dlg_set;

    current_dlg_set.speed_unit = (Unit)GetValueInteger((unsigned)UnitsSpeed);
    current_dlg_set.wind_speed_unit = current_dlg_set.speed_unit;
    current_dlg_set.distance_unit = (Unit)GetValueInteger((unsigned)UnitsDistance);
    current_dlg_set.vertical_speed_unit = (Unit)GetValueInteger((unsigned)UnitsLift);
    current_dlg_set.altitude_unit = (Unit)GetValueInteger((unsigned)UnitsAltitude);
    current_dlg_set.temperature_unit = (Unit)GetValueInteger((unsigned)UnitsTemperature);
    current_dlg_set.task_speed_unit = (Unit)GetValueInteger((unsigned)UnitsTaskSpeed);
    current_dlg_set.pressure_unit = (Unit)GetValueInteger((unsigned)UnitsPressure);

    LoadValueEnum(UnitsPreset, Units::Store::EqualsPresetUnits(current_dlg_set));
    break;
  }

  case DataField::daSpecial:
    return;
  }
}

static void
OnFieldData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  instance->PresetCheck(Mode);
}

void
UnitsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const UnitSetting &config = CommonInterface::GetUISettings().units;
  const CoordinateFormat coordinate_format =
      CommonInterface::GetUISettings().coordinate_format;

  RowFormWidget::Prepare(parent, rc);
  instance = this;

  static const TCHAR * preset_help = N_("Load a set of units.");
  WndProperty *wp = AddEnum(_("Preset"), _T(""));
  DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
  df.EnableItemHelp(true);

  df.addEnumText(gettext(custom_preset_label), (unsigned)0, _("My individual set of units."));
  unsigned len = Units::Store::Count();
  for (unsigned i = 0; i < len; i++)
    df.addEnumText(Units::Store::GetName(i), i+1, preset_help);

  LoadValueEnum(UnitsPreset, Units::Store::EqualsPresetUnits(config));
  wp->GetDataField()->SetDataAccessCallback(OnUnitsPreset);

  AddSpacer();

  // Should be all Expert items (TODO)
  static const TCHAR * units_speed_help = N_("Units used for airspeed and ground speed.  "
      "A separate unit is available for task speeds.");
  static const StaticEnumChoice  units_speed_list[] = {
    { (unsigned)Unit::STATUTE_MILES_PER_HOUR,  _T("mph"), units_speed_help },
    { (unsigned)Unit::KNOTS,                N_("knots"), units_speed_help },
    { (unsigned)Unit::KILOMETER_PER_HOUR,     _T("km/h"), units_speed_help },
    { 0 }
  };
  AddEnum(_("Aircraft/Wind speed"), _T(""), units_speed_list, (unsigned int)config.speed_unit, OnFieldData);

  static const TCHAR *units_distance_help = _("Units used for horizontal distances e.g. "
      "range to waypoint, distance to go.");
  static const StaticEnumChoice  units_distance_list[] = {
    { (unsigned)Unit::STATUTE_MILES,  _T("sm"), units_distance_help },
    { (unsigned)Unit::NAUTICAL_MILES, _T("nm"), units_distance_help },
    { (unsigned)Unit::KILOMETER,     _T("km"), units_distance_help },
    { 0 }
  };
  AddEnum(_("Distance"), _T(""), units_distance_list,
          (unsigned)config.distance_unit, OnFieldData);

  static const TCHAR *units_lift_help = _("Units used for vertical speeds (variometer).");
  static const StaticEnumChoice  units_lift_list[] = {
    { (unsigned)Unit::KNOTS,          N_("knots"), units_lift_help },
    { (unsigned)Unit::METER_PER_SECOND, _T("m/s"), units_lift_help },
    { (unsigned)Unit::FEET_PER_MINUTE,  _T("ft/min"), units_lift_help },
    { 0 }
  };
  AddEnum(_("Lift"), _T(""), units_lift_list,
          (unsigned)config.vertical_speed_unit, OnFieldData);

  static const TCHAR *units_altitude_help = _("Units used for altitude and heights.");
  static const StaticEnumChoice  units_altitude_list[] = {
    { (unsigned)Unit::FEET,  N_("foot"), units_altitude_help },
    { (unsigned)Unit::METER, N_("meter"), units_altitude_help },
    { 0 }
  };
  AddEnum(_("Altitude"), _T(""), units_altitude_list,
          (unsigned)config.altitude_unit, OnFieldData);

  static const TCHAR *units_temperature_help = _("Units used for temperature.");
  static const StaticEnumChoice  units_temperature_list[] = {
    { (unsigned)Unit::DEGREES_CELCIUS,    _T(DEG "C"), units_temperature_help },
    { (unsigned)Unit::DEGREES_FAHRENHEIT, _T(DEG "F"), units_temperature_help },
    { 0 }
  };
  AddEnum(_("Temperature"), _T(""), units_temperature_list,
          (unsigned)config.temperature_unit, OnFieldData);

  static const TCHAR *units_taskspeed_help = _("Units used for task speeds.");
  static const StaticEnumChoice  units_taskspeed_list[] = {
    { (unsigned)Unit::STATUTE_MILES_PER_HOUR,  _T("mph"), units_taskspeed_help },
    { (unsigned)Unit::KNOTS,                N_("knots"), units_taskspeed_help },
    { (unsigned)Unit::KILOMETER_PER_HOUR,     _T("km/h"), units_taskspeed_help },
    { 0 }
  };
  AddEnum(_("Task speed"), _T(""), units_taskspeed_list,
          (unsigned)config.task_speed_unit, OnFieldData);

  static const TCHAR *units_pressure_help = _("Units used for pressures.");
  static const StaticEnumChoice pressure_labels_list[] = {
    { (unsigned)Unit::HECTOPASCAL, _T("hPa"), units_pressure_help },
    { (unsigned)Unit::MILLIBAR,    _T("mb"), units_pressure_help },
    { (unsigned)Unit::INCH_MERCURY, _T("inHg"), units_pressure_help },
    { 0 }
  };
  AddEnum(_("Pressure"), _T(""), pressure_labels_list,
          (unsigned)config.pressure_unit, OnFieldData);

  AddSpacer();

  static const TCHAR *units_lat_lon_help = _("Units used for latitude and longitude.");
  static const StaticEnumChoice units_lat_lon_list[] = {
    { (unsigned)CoordinateFormat::DDMMSS, _T("DDMMSS"), units_lat_lon_help },
    { (unsigned)CoordinateFormat::DDMMSS_SS, _T("DDMMSS.ss"), units_lat_lon_help },
    { (unsigned)CoordinateFormat::DDMM_MMM, _T("DDMM.mmm"), units_lat_lon_help },
    { (unsigned)CoordinateFormat::DD_DDDD, _T("DD.dddd"), units_lat_lon_help },
    { (unsigned)CoordinateFormat::UTM, _T("UTM"), units_lat_lon_help },
    { 0 }
  };
  AddEnum(_("Lat./Lon."), _T(""), units_lat_lon_list,
          (unsigned)coordinate_format);
}

bool
UnitsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  UnitSetting &config = CommonInterface::SetUISettings().units;
  CoordinateFormat &coordinate_format =
      CommonInterface::SetUISettings().coordinate_format;

  /* the Units settings affect how other form values are read and translated
   * so changes to Units settings should be processed after all other form settings
   */
  changed |= SaveValueEnum(UnitsSpeed, szProfileSpeedUnitsValue, config.speed_unit);
  config.wind_speed_unit = config.speed_unit; // Mapping the wind speed to the speed unit

  changed |= SaveValueEnum(UnitsDistance, szProfileDistanceUnitsValue, config.distance_unit);

  changed |= SaveValueEnum(UnitsLift, szProfileLiftUnitsValue, config.vertical_speed_unit);

  changed |= SaveValueEnum(UnitsAltitude, szProfileAltitudeUnitsValue, config.altitude_unit);

  changed |= SaveValueEnum(UnitsTemperature, szProfileTemperatureUnitsValue, config.temperature_unit);

  changed |= SaveValueEnum(UnitsTaskSpeed, szProfileTaskSpeedUnitsValue, config.task_speed_unit);

  changed |= SaveValueEnum(UnitsPressure, szProfilePressureUnitsValue, config.pressure_unit);

  changed |= SaveValueEnum(UnitsLatLon, szProfileLatLonUnits, coordinate_format);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateUnitsConfigPanel()
{
  return new UnitsConfigPanel();
}

