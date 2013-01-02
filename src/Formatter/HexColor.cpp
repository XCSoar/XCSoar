/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "HexColor.hpp"
#include "Screen/Color.hpp"
#include "Util/NumberParser.hpp"
#include "Compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void
FormatHexColor(TCHAR *buffer, size_t size, const Color _color)
{
  assert(size >= 7);

#if defined(__ARM_ARCH_7A__) && GCC_VERSION < 40500
  // NOTE: The local "c" copy of "color" works around an android compiler
  //       bug (observed with gcc version 4.4.3)
  const Color color(_color.Red(), _color.Green(), _color.Blue());
#else
  const Color color(_color);
#endif
  _sntprintf(buffer, size, _T("#%02X%02X%02X"),
             color.Red(), color.Green(), color.Blue());
}

bool
ParseHexColor(const TCHAR *buffer, Color &color)
{
  if (*buffer != _T('#'))
    return false;

  buffer++;

  TCHAR *endptr;
  unsigned value = ParseUnsigned(buffer, &endptr, 16);
  if (endptr != buffer + 6)
    return false;

  uint8_t r = value >> 16;
  uint8_t g = value >> 8;
  uint8_t b = value;

  color = Color(r, g, b);
  return true;
}
