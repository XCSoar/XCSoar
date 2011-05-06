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
#include "Interface.hpp"

void
DrawHorizon(Canvas &canvas, const PixelRect &rc)
{
  /*
  FEATURE TEMPORARILY DISABLED DUE TO USE OF XCSOAR IN FAI COMPETITIONS

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

  RasterPoint Start;
  Start.y = IBLSCALE(55) + rc.top;
  Start.x = rc.right - IBLSCALE(19);

  Pen hpHorizonSky(IBLSCALE(1), dark_color(Graphics::skyColor));
  Brush hbHorizonSky(Graphics::skyColor);
  Pen hpHorizonGround(IBLSCALE(1), Graphics::GroundColor);

#define fixed_div fixed(1.0 / 50.0)
#define fixed_89 fixed_int_constant(89)

  int radius = IBLSCALE(17);
  fixed phi = max(-fixed_89, min(fixed_89,
      XCSoarInterface::Basic().acceleration.BankAngle.value_degrees()));
  fixed alpha = fixed_rad_to_deg * acos(max(-fixed_one, min(fixed_one,
      XCSoarInterface::Basic().acceleration.PitchAngle.value_degrees() * fixed_div)));
  fixed sphi = fixed_180 - phi;
  Angle alpha1 = Angle::degrees(sphi - alpha);
  Angle alpha2 = Angle::degrees(sphi + alpha);

  canvas.select(hpHorizonSky);
  canvas.select(hbHorizonSky);

  canvas.segment(Start.x, Start.y, radius, alpha2, alpha1, true);

  canvas.select(hpHorizonGround);
  canvas.select(Graphics::hbGround);

  canvas.segment(Start.x, Start.y, radius, alpha1, alpha2, true);

  Pen dash_pen(Pen::DASH, 2, COLOR_BLACK);
  canvas.select(dash_pen);

  canvas.line(Start.x + radius / 2, Start.y, Start.x - radius / 2, Start.y);
  canvas.line(Start.x, Start.y - radius / 4, Start.x - radius / 2, Start.y);

  unsigned rr2p = uround(radius * fixed_sqrt_half) + IBLSCALE(1);
  unsigned rr2n = uround(radius * fixed_sqrt_half);

  canvas.black_pen();
  canvas.line(Start.x + rr2p, Start.y - rr2p, Start.x + rr2n, Start.y - rr2n);
  canvas.line(Start.x - rr2p, Start.y - rr2p, Start.x - rr2n, Start.y - rr2n);

  if (XCSoarInterface::Basic().StallRatioAvailable) {
    // JMW experimental, display stall sensor
    fixed s = max(fixed_zero, min(fixed_one, XCSoarInterface::Basic().StallRatio));
    long m = (long)((rc.bottom - rc.top) * s * s);

    canvas.black_pen();
    canvas.line(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
