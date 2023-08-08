// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/HorizonLook.hpp"
#include "NMEA/Attitude.hpp"
#include "Math/Constants.hpp"
#include "Math/Util.hpp"
#include "util/Clamp.hpp"

#include <algorithm>

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

  const auto center = rc.GetCenter();

  const int radius = std::min(rc.GetWidth(), rc.GetHeight()) / 2
    - Layout::Scale(1);

  auto bank_degrees = attitude.bank_angle_available
    ? attitude.bank_angle.Degrees()
    : 0.;

  auto pitch_degrees = attitude.pitch_angle_available
    ? attitude.pitch_angle.Degrees()
    : 0.;

  auto cosine_ratio = pitch_degrees / 50;
  auto alpha = Angle::acos(Clamp(cosine_ratio,
                                 -1., 1.));
  auto sphi = Angle::HalfCircle() - Angle::Degrees(bank_degrees);
  auto alpha1 = sphi - alpha;
  auto alpha2 = sphi + alpha;

  // draw sky part
  if (cosine_ratio > -1 ) { // when less than -1 then the sky is not visible
    canvas.Select(look.sky_pen);
    canvas.Select(look.sky_brush);
    canvas.DrawSegment(center, radius, alpha2, alpha1, true);
  }

  // draw ground part
  if (cosine_ratio < 1) { // when greater than 1 then the ground is not visible
    canvas.Select(look.terrain_pen);
    canvas.Select(look.terrain_brush);
    canvas.DrawSegment(center, radius, alpha1, alpha2, true);
  }
  
  // draw aircraft symbol
  canvas.Select(look.aircraft_pen);
  canvas.DrawLine({center.x + radius / 2, center.y}, {center.x - radius / 2, center.y});
  canvas.DrawLine({center.x, center.y - radius / 4}, {center.x, center.y});

  // draw 45 degree dash marks
  const int rr2p = uround(radius * M_SQRT1_2) + Layout::Scale(1);
  const int rr2n = rr2p - Layout::Scale(2);
  canvas.DrawLine({center.x + rr2p, center.y - rr2p},
                  {center.x + rr2n, center.y - rr2n});
  canvas.DrawLine({center.x - rr2p, center.y - rr2p},
                  {center.x - rr2n, center.y - rr2n});
}
