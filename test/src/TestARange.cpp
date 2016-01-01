/* Copyright_License {

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

#include "Math/ARange.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  plan_tests(70);

  /* basic test with an empty range at 0 */
  constexpr AngleRange zero(Angle::Zero(), Angle::Zero());
  ok1(equals(zero.GetLength(), 0));
  ok1(equals(zero.GetMiddle(), 0));
  ok1(zero.IsInside(Angle::Zero()));
  ok1(!zero.IsInside(Angle::Degrees(1)));

  /* basic test with an empty range at 90 */
  constexpr AngleRange empty(Angle::QuarterCircle(),
                             Angle::QuarterCircle());
  ok1(equals(empty.GetLength(), 0));
  ok1(equals(empty.GetMiddle(), 90));
  ok1(!empty.IsInside(Angle::Zero()));
  ok1(empty.IsInside(Angle::QuarterCircle()));

  /* non-empty range with Extend() */
  AngleRange a(Angle::QuarterCircle(),
               Angle::HalfCircle());
  ok1(a.IsInside(Angle::QuarterCircle()));
  ok1(a.IsInside(Angle::HalfCircle()));
  ok1(a.IsInside(Angle::Degrees(135)));
  ok1(!a.IsInside(Angle::Degrees(89)));
  ok1(a.IsInside(Angle::Degrees(91)));
  ok1(a.IsInside(Angle::Degrees(179)));
  ok1(!a.IsInside(Angle::Degrees(181)));
  ok1(!a.IsInside(Angle::Zero()));
  ok1(equals(a.GetMiddle(), 135));
  ok1(equals(a.GetLength(), 90));

  a.Extend(Angle::Degrees(90));
  ok1(equals(a.start, 90));
  ok1(equals(a.end, 180));

  a.Extend(Angle::Degrees(180));
  ok1(equals(a.start, 90));
  ok1(equals(a.end, 180));

  a.Extend(Angle::Degrees(135));
  ok1(equals(a.start, 90));
  ok1(equals(a.end, 180));

  a.Extend(Angle::Degrees(45));
  ok1(equals(a.start, 45));
  ok1(equals(a.end, 180));
  ok1(equals(a.GetLength(), 135));

  a.Extend(Angle::Degrees(200));
  ok1(equals(a.start, 45));
  ok1(equals(a.end, 200));
  ok1(equals(a.GetLength(), 155));

  /* wrap around 360/0 */
  AngleRange b(Angle::Degrees(350), Angle::Degrees(20));
  ok1(b.IsInside(Angle::Degrees(355)));
  ok1(b.IsInside(Angle::Zero()));
  ok1(b.IsInside(Angle::Degrees(5)));
  ok1(b.IsInside(Angle::Degrees(20)));
  ok1(!b.IsInside(Angle::Degrees(21)));
  ok1(!b.IsInside(Angle::Degrees(349)));
  ok1(equals(b.GetLength(), 30));
  ok1(equals(b.GetMiddle().AsBearing(), 5));

  /* wrap around 180/-180 */
  AngleRange c(Angle::Degrees(170), Angle::Degrees(-160));
  ok1(c.IsInside(Angle::Degrees(175)));
  ok1(!c.IsInside(Angle::Degrees(165)));
  ok1(c.IsInside(Angle::Degrees(-165)));
  ok1(!c.IsInside(Angle::Degrees(-155)));
  ok1(equals(c.GetLength(), 30));
  ok1(equals(c.GetMiddle().AsDelta(), -175));

  /* extend to wrap around 360/0 */
  AngleRange d(Angle::Degrees(350), Angle::Degrees(350));

  d.Extend(Angle::Degrees(350));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 350));

  d.Extend(Angle::Degrees(20));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  d.Extend(Angle::Degrees(355));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  d.Extend(Angle::Degrees(0));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  d.Extend(Angle::Degrees(15));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  d.Extend(Angle::Degrees(20));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  AngleRange e(Angle::Degrees(20), Angle::Degrees(20));
  d.Extend(Angle::Degrees(350));
  ok1(equals(d.start, 350));
  ok1(equals(d.end, 20));

  /* extend to wrap around 180/-180 */
  AngleRange f(Angle::Degrees(170), Angle::Degrees(170));
  f.Extend(Angle::Degrees(-160));
  ok1(equals(f.start, 170));
  ok1(equals(f.end, -160));

  f.Extend(Angle::Degrees(170));
  ok1(equals(f.start, 170));
  ok1(equals(f.end, -160));

  f.Extend(Angle::Degrees(-160));
  ok1(equals(f.start, 170));
  ok1(equals(f.end, -160));

  f.Extend(Angle::Degrees(175));
  ok1(equals(f.start, 170));
  ok1(equals(f.end, -160));

  f.Extend(Angle::Degrees(-175));
  ok1(equals(f.start, 170));
  ok1(equals(f.end, -160));

  /* extend to wrap around 180/-180 */
  AngleRange g(Angle::Degrees(-160), Angle::Degrees(-160));
  g.Extend(Angle::Degrees(170));
  ok1(equals(g.start, 170));
  ok1(equals(g.end, -160));

  return exit_status();
}
