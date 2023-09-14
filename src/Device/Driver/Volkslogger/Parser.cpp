// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

using std::string_view_literals::operator""sv;

// RMN: Volkslogger
// Source data:
// $PGCS,1,0EC0,FFF9,0C6E,02*61
// $PGCS,1,0EC0,FFFA,0C6E,03*18
static bool
vl_PGCS1(NMEAInputLine &line, NMEAInfo &info)
{
  if (line.Read(1) != 1)
    return false;

  /* pressure sensor */
  line.Skip();

  // four characers, hex, barometric altitude
  unsigned u_altitude;
  if (line.ReadHexChecked(u_altitude)) {
    int altitude(u_altitude);
    if (altitude > 60000)
      /* Assuming that altitude has wrapped around.  60 000 m occurs
         at QNH ~2000 hPa */
      altitude -= 65535;

    info.ProvidePressureAltitude(altitude);
  }

  // ExtractParameter(String,ctemp,3);
  // four characters, hex, constant.  Value 1371 (dec)

  // nSatellites = (int)(min(12,HexStrToDouble(ctemp, nullptr)));

  return false;
}

bool
VolksloggerDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$PGCS"sv)
    return vl_PGCS1(line, info);
  else
    return false;
}
