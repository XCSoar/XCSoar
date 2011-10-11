/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FLARM_ID_HPP
#define XCSOAR_FLARM_ID_HPP

#include <stdint.h>
#include <tchar.h>

/**
 * The identification number of a FLARM traffic.
 */
class FlarmId {
  enum {
    UNDEFINED_VALUE = 0,
  };

  uint32_t value;

public:
  bool IsDefined() const {
    return value != UNDEFINED_VALUE;
  }

  void Clear() {
    value = UNDEFINED_VALUE;
  }

  bool operator==(FlarmId other) const {
    return value == other.value;
  }

  bool operator<(FlarmId other) const {
    return value < other.value;
  }

  void Parse(const char *input, char **endptr_r);
#ifdef _UNICODE
  void Parse(const TCHAR *input, TCHAR **endptr_r);
#endif

  const TCHAR *Format(TCHAR *buffer) const;
};

#endif
