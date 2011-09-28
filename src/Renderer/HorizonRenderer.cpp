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

#include "HorizonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Graphics.hpp"
#include "NMEA/Info.hpp"

void
HorizonRenderer::Draw(Canvas &canvas, const PixelRect &rc, const NMEAInfo &Basic)
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

  Pen hpHorizonSky(Layout::Scale(1), dark_color(Graphics::skyColor));
  Brush hbHorizonSky(Graphics::skyColor);
  Pen hpHorizonGround(Layout::Scale(1), dark_color(Graphics::GroundColor));

#define fixed_div fixed(1.0 / 50.0)
#define fixed_89 fixed_int_constant(89)

  fixed phi = max(-fixed_89,
                  min(fixed_89, Basic.acceleration.bank_angle.Degrees()));
  fixed alpha = fixed_rad_to_deg * acos(max(-fixed_one,min(fixed_one,
                  Basic.acceleration.pitch_angle.Degrees() * fixed_div)));
  fixed sphi = fixed_180 - phi;
  Angle alpha1 = Angle::Degrees(sphi - alpha);
  Angle alpha2 = Angle::Degrees(sphi + alpha);

  // draw sky part
  canvas.select(hpHorizonSky);
  canvas.select(hbHorizonSky);
  canvas.segment(center.x, center.y, radius, alpha2, alpha1, true);

  // draw ground part
  canvas.select(hpHorizonGround);
  canvas.select(Graphics::hbGround);
  canvas.segment(center.x, center.y, radius, alpha1, alpha2, true);

  // draw aircraft symbol
  Pen aircraft_pen(Layout::Scale(2), COLOR_BLACK);
  canvas.select(aircraft_pen);
  canvas.line(center.x + radius / 2, center.y, center.x - radius / 2, center.y);
  canvas.line(center.x, center.y - radius / 4, center.x, center.y);

  // draw 45 degree dash marks
  const unsigned rr2p = uround(radius * fixed_sqrt_half) + Layout::Scale(1);
  const unsigned rr2n = rr2p - Layout::Scale(2);
  canvas.line(center.x + rr2p, center.y - rr2p,
              center.x + rr2n, center.y - rr2n);
  canvas.line(center.x - rr2p, center.y - rr2p,
              center.x - rr2n, center.y - rr2n);
}
