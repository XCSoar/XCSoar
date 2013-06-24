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

#ifndef XCSOAR_HEX_COLOR_FORMATTER_HPP
#define XCSOAR_HEX_COLOR_FORMATTER_HPP

#ifdef _UNICODE
#include <tchar.h>
#endif

#include <stddef.h>

class RGB8Color;

/**
 * Formats a Color struct into a hex-based RGB string, i.e. "#123456"
 */
void
FormatHexColor(char *buffer, size_t size, const RGB8Color color);

bool
ParseHexColor(const char *buffer, RGB8Color &color);

#ifdef _UNICODE

bool
ParseHexColor(const TCHAR *buffer, RGB8Color &color);

#endif

#endif
