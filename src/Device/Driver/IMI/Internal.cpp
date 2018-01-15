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

#include "Internal.hpp"
#include "Protocol/Protocol.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Unit.hpp"
#include "Units/Units.hpp"
#include "NMEA/Checksum.hpp"
#include "Util/StringAPI.hxx"

bool
IMIDevice::Connect(OperationEnvironment &env)
{
  // connect to the device
  if (!IMI::Connect(port, env))
    return false;

  return true;
}

void
IMIDevice::Disconnect(OperationEnvironment &env)
{
  // disconnect
  IMI::Disconnect(port, env);
}

static bool
ReadAltitude(NMEAInputLine &line, double &value_r)
{
  double value;
  bool available = line.ReadChecked(value);
  char unit = line.ReadFirstChar();
  if (!available)
    return false;

  if (unit == _T('f') || unit == _T('F'))
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
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PGRMZ")) {
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
