/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "HorizonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/HorizonLook.hpp"
#include "NMEA/Attitude.hpp"
#include "Util/Clamp.hpp"

void
HorizonRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                      const HorizonLook &look,
                      const AttitudeState &attitude)
{
  /*
  This feature of having a backup artificial horizon based on inferred
  orientation from GPS and vario data is useful, and reasonably well
  tested, but has the issue of potentially invalidating use of XCSoar in
  FAI contests due to rule ref Annex A to Section 3 (2010 Edition) 4.1.2
  "No instruments permitting pilots to fly without visual reference to
  the ground may be carried on board, even if made unserviceable."  The
  quality of XCSoar's pseudo-AH is arguably good enough that this
  violates the rule.  We need to seek clarification as to whether this
  is the case or not.
  */

  RasterPoint center;
  center.y = (rc.top + rc.bottom) / 2;
  center.x = (rc.left + rc.right) / 2;
  const int radius = min(rc.right - rc.left, rc.bottom - rc.top) / 2 -
                     Layout::Scale(1);

#define fixed_div fixed(1.0 / 50.0)

  fixed bank_degrees = attitude.bank_angle_available ?
                       attitude.bank_angle.Degrees() : fixed(0);

  fixed pitch_degrees = attitude.pitch_angle_available ?
                        attitude.pitch_angle.Degrees() : fixed(0);

  fixed phi = Clamp(bank_degrees, fixed(-89), fixed(89));
  Angle alpha = Angle::Radians(acos(Clamp(pitch_degrees * fixed_div,
                                          fixed(-1), fixed(1))));
  Angle sphi = Angle::HalfCircle() - Angle::Degrees(phi);
  Angle alpha1 = sphi - alpha;
  Angle alpha2 = sphi + alpha;

  // draw sky part
  canvas.Select(look.sky_pen);
  canvas.Select(look.sky_brush);
  canvas.DrawSegment(center.x, center.y, radius, alpha2, alpha1, true);

  // draw ground part
  canvas.Select(look.terrain_pen);
  canvas.Select(look.terrain_brush);
  canvas.DrawSegment(center.x, center.y, radius, alpha1, alpha2, true);

  // draw aircraft symbol
  canvas.Select(look.aircraft_pen);
  canvas.DrawLine(center.x + radius / 2, center.y, center.x - radius / 2, center.y);
  canvas.DrawLine(center.x, center.y - radius / 4, center.x, center.y);

  // draw 45 degree dash marks
  const UPixelScalar rr2p = uround(radius * fixed_sqrt_half) + Layout::Scale(1);
  const UPixelScalar rr2n = rr2p - Layout::Scale(2);
  canvas.DrawLine(center.x + rr2p, center.y - rr2p,
              center.x + rr2n, center.y - rr2n);
  canvas.DrawLine(center.x - rr2p, center.y - rr2p,
              center.x - rr2n, center.y - rr2n);
}
