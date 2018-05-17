/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "UnitsEditor.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

void
CAI302UnitsEditor::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  static constexpr StaticEnumChoice vario_list[] = {
    { 0, _T("m/s"), },
    { 1, _T("kt"), },
    { 0 }
  };
  AddEnum(_("Vario"), NULL, vario_list,
          data.GetVarioUnit());

  static constexpr StaticEnumChoice altitude_list[] = {
    { 0, _T("m"), },
    { 1, _T("ft"), },
    { 0 }
  };
  AddEnum(_("Altitude"), NULL, altitude_list,
          data.GetAltitudeUnit());

  static constexpr StaticEnumChoice temperature_list[] = {
    { 0, _T(DEG "C"), },
    { 1, _T(DEG "F"), },
    { 0 }
  };
  AddEnum(_("Temperature"), NULL, temperature_list,
          data.GetTemperatureUnit());

  static constexpr StaticEnumChoice pressure_list[] = {
    { 0, _T("hPa"), },
    { 1, _T("inHg"), },
    { 0 }
  };
  AddEnum(_("Pressure"), NULL, pressure_list,
          data.GetPressureUnit());

  static constexpr StaticEnumChoice distance_list[] = {
    { 0, _T("km"), },
    { 1, _T("NM"), },
    { 2, _T("mi"), },
    { 0 }
  };
  AddEnum(_("Distance"), NULL, distance_list,
          data.GetDistanceUnit());

  static constexpr StaticEnumChoice speed_list[] = {
    { 0, _T("m/s"), },
    { 1, _T("kt"), },
    { 2, _T("mph"), },
    { 0 }
  };
  AddEnum(_("Speed"), NULL, speed_list,
          data.GetSpeedUnit());

  static constexpr StaticEnumChoice sink_tone_list[] = {
    { 0, _T("off"), },
    { 1, _T("on"), },
    { 0 }
  };
  AddEnum(_("Sink tone"), NULL, sink_tone_list,
          data.GetSinkTone());
}

bool
CAI302UnitsEditor::Save(bool &_changed)
{
  bool changed = false;

  unsigned vario = data.GetVarioUnit();
  if (SaveValue(VarioUnit, vario)) {
    data.SetVarioUnit(vario);
    changed = true;
  }

  unsigned altitude = data.GetAltitudeUnit();
  if (SaveValue(AltitudeUnit, altitude)) {
    data.SetAltitudeUnit(altitude);
    changed = true;
  }

  unsigned temperature = data.GetTemperatureUnit();
  if (SaveValue(TemperatureUnit, temperature)) {
    data.SetTemperatureUnit(temperature);
    changed = true;
  }

  unsigned pressure = data.GetPressureUnit();
  if (SaveValue(PressureUnit, pressure)) {
    data.SetPressureUnit(pressure);
    changed = true;
  }

  unsigned distance = data.GetDistanceUnit();
  if (SaveValue(DistanceUnit, distance)) {
    data.SetDistanceUnit(distance);
    changed = true;
  }

  unsigned speed = data.GetSpeedUnit();
  if (SaveValue(SpeedUnit, speed)) {
    data.SetSpeedUnit(speed);
    changed = true;
  }

  unsigned sink_tone = data.GetSinkTone();
  if (SaveValue(SinkTone, sink_tone)) {
    data.SetSinkTone(sink_tone);
    changed = true;
  }

  _changed |= changed;
  return true;
}
