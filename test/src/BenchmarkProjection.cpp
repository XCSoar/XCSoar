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

#include "Projection/Projection.hpp"
#include "Screen/Layout.hpp"

unsigned Layout::scale_1024 = 1024;

class TestProjection : public Projection {
public:
  TestProjection() {
    SetScreenOrigin(0, 0);
    SetScale(640. / (100 * 2));
    SetGeoLocation(GeoPoint(Angle::Degrees(7.7061111111111114),
                            Angle::Degrees(51.051944444444445)));
  }
};

int main(int argc, char **argv)
{
  TestProjection projection;

  GeoPoint gp = GeoPoint(Angle::Degrees(7.7061111111111114),
                         Angle::Degrees(51.051944444444445));
  long x = 0, y = 0;
  for (unsigned i = 64 * 1024 * 1024; i-- > 0;) {
    auto rp = projection.GeoToScreen(gp);

    /* prevent gcc from optimizing this loop away */
    x += rp.x;
    y += rp.y;
  }

  return x + y;
}
