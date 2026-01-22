// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/LoEFGREN.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"

using std::string_view_literals::operator""sv;

class LoEFGRENDevice : public AbstractDevice {
public:
  LoEFGRENDevice() = default;

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
PLOF(NMEAInputLine &line, NMEAInfo &info)
{
  /* $PLOF,<TE compensated climb/sink in cm/s>,<indicated airspeed km/h>,
   * <temperature in deg c x 10>*
   *
   * TE compensated climb/sink = variometer in cm/s (up positive)
   * indicated airspeed = indicated airspeed in km/h
   * temperature = temperature in degrees Celsius x 10 (e.g., 15.2°C = 152)
   *
   * Protocol documentation:
   * https://www.lofgren-electronics.fr/Lofgren%20Variometer%20User%20Manual%20EN%20.pdf
   * Section "NMEA output"
   */

  double value;

  // Parse TE compensated variometer (cm/s -> m/s)
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value / 100);

  // Parse indicated airspeed (km/h -> m/s)
  if (line.ReadChecked(value))
    info.ProvideIndicatedAirspeed(Units::ToSysUnit(value,
                                                    Unit::KILOMETER_PER_HOUR));

  // Parse temperature (°C x 10 -> °C)
  if (line.ReadChecked(value)) {
    info.temperature = Temperature::FromCelsius(value / 10);
    info.temperature_available = true;
  }

  return true;
}

bool
LoEFGRENDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if (line == nullptr)
    return false;

  if (!VerifyNMEAChecksum(line))
    return false;

  NMEAInputLine input(line);
  const auto type = input.ReadView();

  if (type == "$PLOF"sv)
    return PLOF(input, info);

  return false;
}

static Device *
LoEFGRENCreateOnPort([[maybe_unused]] const DeviceConfig &config,
                     [[maybe_unused]] Port &port)
{
  return new LoEFGRENDevice();
}

const struct DeviceRegister loe_fgren_driver = {
  _T("LoEFGREN"),
  _T("LöFGREN Variometer"),
  0,
  LoEFGRENCreateOnPort,
};

