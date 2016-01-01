/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#define USE_FREETYPE 1
#define ENABLE_OPENGL 1
#include "Screen/OpenGL/Canvas.hpp"
#include "Math/Angle.hpp"
#include "Math/FastTrig.hpp"

class Canvas;

#include <iostream>

void Test();

//------------------------------------------------------------------------------
void
Test()
  {
  Canvas c;

  c.ClearWhite();
  c.DrawKeyhole(0, 0, 5000, 10000, Angle::Degrees(350), Angle::Degrees(10));
  }

//------------------------------------------------------------------------------
int
main(int argc, const char *argv[])
  {

  Test();

  return 0;
  }
