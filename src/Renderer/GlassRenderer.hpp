/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_GLASS_RENDERER_HPP
#define XCSOAR_GLASS_RENDERER_HPP

class Canvas;
struct PixelRect;
class Color;

/**
 * Fill the background with the specified color and a glass-like
 * effect.
 *
 * This is only implemented on OpenGL when the specified color is
 * bright white.  The compile-time option "EYE_CANDY" must be enabled.
 * In all other cases, the background is simply filled with the
 * specified color, without any additional effect.
 */
void
DrawGlassBackground(Canvas &canvas, const PixelRect &rc, Color color);

#endif
