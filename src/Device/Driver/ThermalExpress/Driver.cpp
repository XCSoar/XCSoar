// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/ThermalExpress/Driver.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"

using std::string_view_literals::operator""sv;

bool
ThermalExpressDevice::ParseTXP(NMEAInputLine &line, NMEAInfo &info)
{
  // $TXP,altitude,vario
  double altitude;
  bool alt_available = line.ReadChecked(altitude);
  if (alt_available && (altitude >= -10000 && altitude <= 10000))
     info.ProvideBaroAltitudeTrue(altitude);
  
  double vario;
  bool vario_available = line.ReadChecked(vario);
  if (vario_available && (vario >= -5000 && vario <= 5000))
    info.ProvideTotalEnergyVario(vario/100.);

  return true; 
}

bool
ThermalExpressDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  NMEAInputLine input_line(line);

  const auto type = input_line.ReadView();
  if (type == "$TXP"sv)
    return ParseTXP(input_line, info);
  else
    return false;
}

static Device *
ThermalExpressCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new ThermalExpressDevice();
}

const struct DeviceRegister thermalexpress_driver = {
  "ThermalExpress",
  "Thermal Express",
  0,
  ThermalExpressCreateOnPort,
};
