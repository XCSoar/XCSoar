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

#include "Engine/Task/Shapes/FAITriangleArea.hpp"
#include "Engine/Task/Shapes/FAITriangleSettings.hpp"
#include "Geo/GeoPoint.hpp"
#include "Compiler.h"

int
main(gcc_unused int argc, gcc_unused char **argv)
{
  FAITriangleSettings settings;
  settings.SetDefaults();

  const GeoPoint a(Angle::Degrees(7.70722),
                   Angle::Degrees(51.052));
  const GeoPoint b(Angle::Degrees(11.5228),
                   Angle::Degrees(50.3972));

  GeoPoint buffer[FAI_TRIANGLE_SECTOR_MAX];

  for (unsigned i = 256 * 1024; i-- > 0;)
    GenerateFAITriangleArea(buffer, a, b, false, settings);

  return 0;
}
