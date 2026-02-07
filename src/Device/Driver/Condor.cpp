// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Condor.hpp"
#include "Device/Driver.hpp"
#include "Units/System.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

using std::string_view_literals::operator""sv;

class CondorDevice : public AbstractDevice {
private:
  bool reciprocal_wind;

public:
  explicit CondorDevice(bool reciprocal = true) : reciprocal_wind(reciprocal) {}
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
cLXWP0(NMEAInputLine &line, NMEAInfo &info, bool reciprocal_wind)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 logger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3 vario (m/s)
   4-8 unknown
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */


  line.Skip();

  double airspeed;
  bool tas_available = line.ReadChecked(airspeed);

  double value;
  if (line.ReadChecked(value))
    info.ProvideBaroAltitudeTrue(value);

  if (tas_available)
    info.ProvideTrueAirspeed(Units::ToSysUnit(airspeed, Unit::KILOMETER_PER_HOUR));

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  line.Skip(6);

  if (SpeedVector wind; line.ReadSpeedVectorKPH(wind)) {
    if (reciprocal_wind) {
      /* Condor 1.1.4 and Condor 2 outputs the direction that the wind is going
       * to, _not_ the direction it is coming from !! This seems to differ from
       * the output that the LX devices are giving !!
       */
      info.ProvideExternalWind(wind.Reciprocal());
    } else {
      /* Condor3 outputs the direction the wind is coming from. */
      info.ProvideExternalWind(wind);
    };
  };
  return true;
}

bool
CondorDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$LXWP0"sv) return cLXWP0(line, info, reciprocal_wind);
  else
    return false;
}

static Device *
CondorCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new CondorDevice(true); // Reciprocal wind enabled
}

const struct DeviceRegister condor_driver = {
  "Condor",
  "Condor Soaring Simulator",
  0,
  CondorCreateOnPort,
};

static Device *
Condor3CreateOnPort([[maybe_unused]] const DeviceConfig &config,
                    [[maybe_unused]] Port &com_port)
{
  return new CondorDevice(false); // Reciprocal wind disabled
}

const struct DeviceRegister condor3_driver = {
    "Condor3",
    "Condor Soaring Simulator 3",
    0,
    Condor3CreateOnPort,
};
