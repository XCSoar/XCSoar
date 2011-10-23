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

#ifndef XCSOAR_SCREEN_COLOR_HPP
#define XCSOAR_SCREEN_COLOR_HPP

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Color.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Color.hpp"
#else
#include "Screen/GDI/Color.hpp"
#endif

#define COLOR_WHITE Color(0xff, 0xff, 0xff)
#define COLOR_BLACK Color(0x00, 0x00, 0x00)
#define COLOR_GRAY Color(0x80, 0x80, 0x80)
#define COLOR_LIGHT_GRAY Color(0xc0, 0xc0, 0xc0)
#define COLOR_DARK_GRAY Color(0x4, 0x40, 0x40)
#define COLOR_RED Color(0xff, 0x00, 0x00)
#define COLOR_GREEN Color(0x00, 0xff, 0x00)
#define COLOR_BLUE Color(0x00, 0x00, 0xff)
#define COLOR_YELLOW Color(0xff, 0xff, 0x00)
#define COLOR_CYAN Color(0x00, 0xff, 0xff)
#define COLOR_MAGENTA Color(0xff, 0x00, 0xff)
#define COLOR_ORANGE Color(0xff, 0xa2, 0x00)
#define COLOR_BROWN Color(0xb7, 0x64, 0x1e)

static inline gcc_constexpr_function uint8_t
LightColor(uint8_t c)
{
  return ((c ^ 0xff) >> 1) ^ 0xff;
}

/**
 * Returns a lighter version of the specified color, adequate for
 * SRCAND filtering.
 */
static inline gcc_constexpr_function Color
LightColor(Color c)
{
  return Color(LightColor(c.Red()), LightColor(c.Green()),
               LightColor(c.Blue()));
}

static inline gcc_constexpr_function uint8_t
DarkColor(uint8_t c)
{
  return (c >> 1);
}

/**
 * Returns a darker version of the specified color.
 */
static inline gcc_constexpr_function Color
DarkColor(Color c)
{
  return Color(DarkColor(c.Red()), DarkColor(c.Green()),
               DarkColor(c.Blue()));
}

Color Desaturate(Color c);

#endif
