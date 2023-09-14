// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnitsEditor.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

void
CAI302UnitsEditor::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  static constexpr StaticEnumChoice vario_list[] = {
    { 0, _T("m/s"), },
    { 1, _T("kt"), },
    nullptr
  };
  AddEnum(_("Vario"), NULL, vario_list,
          data.GetVarioUnit());

  static constexpr StaticEnumChoice altitude_list[] = {
    { 0, _T("m"), },
    { 1, _T("ft"), },
    nullptr
  };
  AddEnum(_("Altitude"), NULL, altitude_list,
          data.GetAltitudeUnit());

  static constexpr StaticEnumChoice temperature_list[] = {
    { 0, _T(DEG "C"), },
    { 1, _T(DEG "F"), },
    nullptr
  };
  AddEnum(_("Temperature"), NULL, temperature_list,
          data.GetTemperatureUnit());

  static constexpr StaticEnumChoice pressure_list[] = {
    { 0, _T("hPa"), },
    { 1, _T("inHg"), },
    nullptr
  };
  AddEnum(_("Pressure"), NULL, pressure_list,
          data.GetPressureUnit());

  static constexpr StaticEnumChoice distance_list[] = {
    { 0, _T("km"), },
    { 1, _T("NM"), },
    { 2, _T("mi"), },
    nullptr
  };
  AddEnum(_("Distance"), NULL, distance_list,
          data.GetDistanceUnit());

  static constexpr StaticEnumChoice speed_list[] = {
    { 0, _T("m/s"), },
    { 1, _T("kt"), },
    { 2, _T("mph"), },
    nullptr
  };
  AddEnum(_("Speed"), NULL, speed_list,
          data.GetSpeedUnit());

  static constexpr StaticEnumChoice sink_tone_list[] = {
    { 0, _T("off"), },
    { 1, _T("on"), },
    nullptr
  };
  AddEnum(_("Sink tone"), NULL, sink_tone_list,
          data.GetSinkTone());
}

bool
CAI302UnitsEditor::Save(bool &_changed) noexcept
{
  bool changed = false;

  unsigned vario = data.GetVarioUnit();
  if (SaveValueEnum(VarioUnit, vario)) {
    data.SetVarioUnit(vario);
    changed = true;
  }

  unsigned altitude = data.GetAltitudeUnit();
  if (SaveValueEnum(AltitudeUnit, altitude)) {
    data.SetAltitudeUnit(altitude);
    changed = true;
  }

  unsigned temperature = data.GetTemperatureUnit();
  if (SaveValueEnum(TemperatureUnit, temperature)) {
    data.SetTemperatureUnit(temperature);
    changed = true;
  }

  unsigned pressure = data.GetPressureUnit();
  if (SaveValueEnum(PressureUnit, pressure)) {
    data.SetPressureUnit(pressure);
    changed = true;
  }

  unsigned distance = data.GetDistanceUnit();
  if (SaveValueEnum(DistanceUnit, distance)) {
    data.SetDistanceUnit(distance);
    changed = true;
  }

  unsigned speed = data.GetSpeedUnit();
  if (SaveValueEnum(SpeedUnit, speed)) {
    data.SetSpeedUnit(speed);
    changed = true;
  }

  unsigned sink_tone = data.GetSinkTone();
  if (SaveValueEnum(SinkTone, sink_tone)) {
    data.SetSinkTone(sink_tone);
    changed = true;
  }

  _changed |= changed;
  return true;
}
