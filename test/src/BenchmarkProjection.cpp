// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

int main()
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
