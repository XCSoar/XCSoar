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

#include "Device.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

#include <string.h>

bool
FlarmDevice::ParsePFLAC(NMEAInputLine &line)
{
  char responsetype[10];
  line.Read(responsetype, 10);

  char name[80];
  line.Read(name, 80);

  if (StringIsEqual(name, "ERROR"))
    // ignore error responses...
    return true;

  char value[256];
  line.Read(value, ARRAY_SIZE(value));

  settings.Lock();
  settings.Set(name, value);
  settings.Unlock();

  return true;
}

bool
FlarmDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PFLAC"))
    return ParsePFLAC(line);
  else
    return false;
}
