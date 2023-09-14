// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/ILEC.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"

using std::string_view_literals::operator""sv;

class ILECDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double norm, bearing;

  bool bearing_valid = line.ReadChecked(bearing);
  bool norm_valid = line.ReadChecked(norm);

  if (bearing_valid && norm_valid) {
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    value_r.bearing = Angle::Degrees(bearing);
    return true;
  } else
    return false;
}

/**
 * Parse a "$PILC,PDA1" sentence.
 *
 * Example: "$PILC,PDA1,1489,-3.21,274,15,58*7D"
 */
static bool
ParsePDA1(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // altitude [m]
  int altitude;
  if (line.ReadChecked(altitude))
    info.ProvideBaroAltitudeTrue(altitude);

  // total energy vario [m/s]
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  // wind direction [degrees, kph]
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  // confidence [0..100]
  // not used

  return true;
}

bool
ILECDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  auto type = line.ReadView();
  if (type == "$PILC"sv) {
    type = line.ReadView();
    if (type == "PDA1"sv)
      return ParsePDA1(line, info);
    else
      return false;
  } else
    return false;
}

static Device *
ILECCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new ILECDevice();
}

const struct DeviceRegister ilec_driver = {
  _T("ILEC SN10"),
  _T("ILEC SN10"),
  0,
  ILECCreateOnPort,
};
