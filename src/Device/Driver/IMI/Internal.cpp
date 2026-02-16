// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol/Protocol.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Unit.hpp"
#include "Units/Units.hpp"
#include "NMEA/Checksum.hpp"

using std::string_view_literals::operator""sv;

bool
IMIDevice::EnableNMEA(OperationEnvironment &env)
{
  IMI::Disconnect(port, env);
  return true;
}

bool
IMIDevice::Connect(OperationEnvironment &env)
{
  // connect to the device
  if (!IMI::Connect(port, env))
    return false;

  return true;
}

static bool
ReadAltitude(NMEAInputLine &line, double &value_r)
{
  double value;
  bool available = line.ReadChecked(value);
  char unit = line.ReadFirstChar();
  if (!available)
    return false;

  if (unit == 'f' || unit == 'F')
    value = Units::ToSysUnit(value, Unit::FEET);

  value_r = value;
  return true;
}

bool
IMIDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$PGRMZ"sv) {
    double value;

    /* The normal Garmin $PGRMZ line contains the "true" barometric
       altitude above MSL (corrected with QNH), but IMIDevice differs:
       it emits the uncorrected barometric altitude (i.e. pressure
       altitude). That is the only reason why we catch this sentence
       in the driver instead of letting the generic class NMEAParser
       do it. (solution inspired by EWMicroRecorderDevice, and
       AltairProDevice. ) */
    if (ReadAltitude(line, value))
      info.ProvidePressureAltitude(value);

    return true;
  } else
    return false;
}
