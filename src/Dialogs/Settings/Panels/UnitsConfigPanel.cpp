// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnitsConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Units/UnitsStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <vector>

static constexpr StaticEnumChoice units_speed_list[] = {
  { Unit::STATUTE_MILES_PER_HOUR, "mph" },
  { Unit::KNOTS, N_("knots") },
  { Unit::KILOMETER_PER_HOUR, "km/h" },
  { Unit::METER_PER_SECOND, "m/s" },
  nullptr
};

static constexpr StaticEnumChoice units_distance_list[] = {
  { Unit::STATUTE_MILES, "sm" },
  { Unit::NAUTICAL_MILES, "nm" },
  { Unit::KILOMETER, "km" },
  nullptr
};

static constexpr StaticEnumChoice units_lift_list[] = {
  { Unit::KNOTS, N_("knots") },
  { Unit::METER_PER_SECOND, "m/s" },
  { Unit::FEET_PER_MINUTE, "ft/min" },
  nullptr
};

static constexpr StaticEnumChoice units_altitude_list[] = {
  { Unit::FEET,  N_("feet") },
  { Unit::METER, N_("meters") },
  nullptr
};

static constexpr StaticEnumChoice units_temperature_list[] = {
  { Unit::DEGREES_CELCIUS, DEG "C" },
  { Unit::DEGREES_FAHRENHEIT, DEG "F" },
  nullptr
};

static constexpr StaticEnumChoice units_taskspeed_list[] = {
  { Unit::STATUTE_MILES_PER_HOUR, "mph" },
  { Unit::KNOTS, N_("knots") },
  { Unit::KILOMETER_PER_HOUR, "km/h" },
  { Unit::METER_PER_SECOND, "m/s" },
  nullptr
};

static constexpr StaticEnumChoice pressure_labels_list[] = {
  { Unit::HECTOPASCAL, "hPa" },
  { Unit::MILLIBAR, "mb" },
  { Unit::INCH_MERCURY, "inHg" },
  nullptr
};

static constexpr StaticEnumChoice mass_labels_list[] = {
  { Unit::KG, "kg" },
  { Unit::LB, "lb" },
  nullptr
};

static constexpr StaticEnumChoice wing_loading_labels_list[] = {
  { Unit::KG_PER_M2, "kg/m²" },
  { Unit::LB_PER_FT2, "lb/ft²" },
  nullptr
};

static constexpr StaticEnumChoice units_lat_lon_list[] = {
  { CoordinateFormat::DDMMSS, "DDMMSS" },
  { CoordinateFormat::DDMMSS_S, "DDMMSS.s" },
  { CoordinateFormat::DDMM_MMM, "DDMM.mmm" },
  { CoordinateFormat::DD_DDDDD, "DD.ddddd" },
  { CoordinateFormat::UTM, "UTM" },
  nullptr
};

static constexpr StaticEnumChoice rotation_labels_list[] = {
  { Unit::HZ, "Hz" },
  { Unit::RPM, "rpm" },
  nullptr
};

/**
 * The units of the numbers XCSoar shows: a preset for all of them,
 * or each on its own.
 */
class UnitsConfigPanel final : public ConfigListPanel {
  UnitSetting units;
  CoordinateFormat coordinate_format;

private:
  /** Let the user pick one of the presets, or keep the custom set. */
  void PickPreset() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
UnitsConfigPanel::LoadSettings() noexcept
{
  const auto &format = CommonInterface::GetUISettings().format;

  units = format.units;
  coordinate_format = format.coordinate_format;
}

void
UnitsConfigPanel::PickPreset() noexcept
{
  /* the custom set first, then the presets: the index of a choice is
     what EqualsPresetUnits() returns */
  std::vector<PickerChoice> choices{
    {_("Custom"), _("My individual set of units.")},
  };

  const unsigned n = Units::Store::Count();
  for (unsigned i = 0; i < n; ++i)
    choices.push_back({Units::Store::GetName(i)});

  const int picked = PickChoice(_("Preset"), _("Load a set of units."),
                                choices,
                                Units::Store::EqualsPresetUnits(units));
  if (picked <= 0)
    /* "Custom" is what the units are already, whichever they are */
    return;

  /* the preset brings the units; the coordinate format and the
     rotation are not part of one */
  const UnitSetting &preset = Units::Store::Read(picked - 1);
  units.speed_unit = preset.speed_unit;
  units.wind_speed_unit = preset.speed_unit;
  units.distance_unit = preset.distance_unit;
  units.vertical_speed_unit = preset.vertical_speed_unit;
  units.altitude_unit = preset.altitude_unit;
  units.temperature_unit = preset.temperature_unit;
  units.task_speed_unit = preset.task_speed_unit;
  units.pressure_unit = preset.pressure_unit;
  units.mass_unit = preset.mass_unit;
  units.wing_loading_unit = preset.wing_loading_unit;

  Refresh();
}

void
UnitsConfigPanel::Fill() noexcept
{
  AddGroup();

  const unsigned preset = Units::Store::EqualsPresetUnits(units);
  AddItem(_("Preset"), [this](){ PickPreset(); },
          {.value = preset > 0 ? Units::Store::GetName(preset - 1) : _("Custom"),
           .chevron = true});

  if (!IsExpert())
    return;

  AddGroup();

  AddEnumItem(_("Aircraft/Wind speed"),
              _("Units used for airspeed and ground speed. "
                "A separate unit is available for task speeds."),
              units_speed_list, units.speed_unit);

  AddEnumItem(_("Distance"),
              _("Units used for horizontal distances e.g. "
                "range to waypoint, distance to go."),
              units_distance_list, units.distance_unit);

  AddEnumItem(_("Lift"), _("Units used for vertical speeds (variometer)."),
              units_lift_list, units.vertical_speed_unit);

  AddEnumItem(_("Altitude"), _("Units used for altitude and heights."),
              units_altitude_list, units.altitude_unit);

  AddEnumItem(_("Temperature"), _("Units used for temperature."),
              units_temperature_list, units.temperature_unit);

  AddEnumItem(_("Task Speed"), _("Units used for task speeds."),
              units_taskspeed_list, units.task_speed_unit);

  AddEnumItem(_("Pressure"), _("Units used for pressures."),
              pressure_labels_list, units.pressure_unit);

  AddEnumItem(_("Mass"), _("Units used for mass."),
              mass_labels_list, units.mass_unit);

  AddEnumItem(_("Wing loading"), _("Units used for wing loading."),
              wing_loading_labels_list, units.wing_loading_unit);

  AddGroup();

  AddEnumItem(_("Lat./Lon."), _("Units used for latitude and longitude."),
              units_lat_lon_list, coordinate_format);

  AddEnumItem(_("Rotation"), _("Unit used for rotation."),
              rotation_labels_list, units.rotation_unit);
}

bool
UnitsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &format = CommonInterface::SetUISettings().format;
  UnitSetting &config = format.units;

  /* the units affect how the values of the other pages are read:
     this page is saved after them */
  changed |= Profile::Update(ProfileKeys::SpeedUnitsValue,
                             config.speed_unit, units.speed_unit);

  /* the wind speed follows the speed unit */
  config.wind_speed_unit = config.speed_unit;

  changed |= Profile::Update(ProfileKeys::DistanceUnitsValue,
                             config.distance_unit, units.distance_unit);
  changed |= Profile::Update(ProfileKeys::LiftUnitsValue,
                             config.vertical_speed_unit,
                             units.vertical_speed_unit);
  changed |= Profile::Update(ProfileKeys::AltitudeUnitsValue,
                             config.altitude_unit, units.altitude_unit);
  changed |= Profile::Update(ProfileKeys::TemperatureUnitsValue,
                             config.temperature_unit, units.temperature_unit);
  changed |= Profile::Update(ProfileKeys::TaskSpeedUnitsValue,
                             config.task_speed_unit, units.task_speed_unit);
  changed |= Profile::Update(ProfileKeys::PressureUnitsValue,
                             config.pressure_unit, units.pressure_unit);
  changed |= Profile::Update(ProfileKeys::MassUnitValue,
                             config.mass_unit, units.mass_unit);
  changed |= Profile::Update(ProfileKeys::WingLoadingUnitValue,
                             config.wing_loading_unit,
                             units.wing_loading_unit);
  changed |= Profile::Update(ProfileKeys::LatLonUnits,
                             format.coordinate_format, coordinate_format);
  changed |= Profile::Update(ProfileKeys::RotationUnitValue,
                             config.rotation_unit, units.rotation_unit);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateUnitsConfigPanel()
{
  return std::make_unique<UnitsConfigPanel>();
}
