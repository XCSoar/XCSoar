/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

#include <string.h>

using std::string_view_literals::operator""sv;

bool
FlarmDevice::ParsePFLAC(NMEAInputLine &line)
{
  [[maybe_unused]] const auto responsetype = line.ReadView();

  const auto name = line.ReadView();

  if (name == "ERROR"sv)
    // ignore error responses...
    return true;

  const auto value = line.ReadView();

  const std::lock_guard<Mutex> lock(settings);
  settings.Set(std::string{name}, value);

  return true;
}

bool
FlarmDevice::ParseNMEA(const char *_line, [[maybe_unused]] NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  const auto type = line.ReadView();
  if (type == "$PFLAC"sv)
    return ParsePFLAC(line);
  else
    return false;
}
