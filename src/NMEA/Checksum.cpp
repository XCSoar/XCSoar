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

#include "NMEA/Checksum.hpp"

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdint.h>

bool
VerifyNMEAChecksum(const char *p)
{
  assert(p != NULL);

  const char *asterisk = strrchr(p, '*');
  if (asterisk == NULL)
    return false;

  const char *checksum_string = asterisk + 1;
  char *endptr;
  unsigned long ReadCheckSum2 = strtoul(checksum_string, &endptr, 16);
  if (endptr == checksum_string || *endptr != 0 || ReadCheckSum2 >= 0x100)
    return false;

  uint8_t ReadCheckSum = (unsigned char)ReadCheckSum2;
  uint8_t CalcCheckSum = NMEAChecksum(p, asterisk - p);

  return CalcCheckSum == ReadCheckSum;
}

void
AppendNMEAChecksum(char *p)
{
  assert(p != NULL);

  sprintf(p + strlen(p), "*%02X", NMEAChecksum(p));
}
