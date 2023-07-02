// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/HorizonLook.hpp"
#include "NMEA/Attitude.hpp"
#include "Math/Constants.hpp"
#include "Math/Util.hpp"

#include <algorithm>

#include <iostream>

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
  auto alpha = Angle::acos(std::clamp(cosine_ratio, -1., 1.));
  auto sphi = Angle::HalfCircle() - Angle::Degrees(bank_degrees);
  auto alpha1 = sphi - alpha;
  auto alpha2 = sphi + alpha;

  // draw sky part
  if (cosine_ratio > -1 ) { // when less than -1 then the sky is not visible
    canvas.Select(look.sky_pen);
    canvas.Select(look.sky_brush);
    canvas.DrawSegment(center, radius, alpha2, alpha1, true);
    // canvas.DrawSegment(center, radius - Layout::Scale(12), alpha2, alpha1, true);
  
  }

  // draw ground part
  if (cosine_ratio < 1) { // when greater than 1 then the ground is not visible
    canvas.Select(look.terrain_pen);
    canvas.Select(look.terrain_brush);
    canvas.DrawSegment(center, radius, alpha1, alpha2, true);
  }

  // draw horizon line
  // auto roll = Angle::Degrees(bank_degrees);
  // auto pitch = Angle::Degrees(pitch_degrees);
  // canvas.Select(look.horizon_pen);
  // std::cout << pitch_degrees << " " <<  pitch.Degrees() << " " << pitch.cos() << " " << pitch.sin() << std::endl;
  // // canvas.DrawText({center.x + radius / 2, center.y}, "toto");
  // int x1 = - radius;
  // int y1 = 0;
  // int x2 = + radius;
  // int y2 = 0;
  // canvas.DrawLine({int(x1*roll.cos()-y1*roll.sin()+center.x), int(x1*roll.sin()+y1*roll.cos()+center.y+radius*pitch.sin())}, 
  //                 {int(x2*roll.cos()-y2*roll.sin()+center.x), int(x2*roll.sin()+y2*roll.cos()+center.y+radius*pitch.sin())});

  
  // draw aircraft symbol
  canvas.Select(look.aircraft_pen);
  canvas.DrawLine({center.x + radius / 2, center.y}, {center.x + radius / 10, center.y});
  canvas.DrawLine({center.x - radius / 2, center.y}, {center.x - radius / 10, center.y});
  canvas.DrawLine({center.x + radius/10, center.y + radius / 10}, {center.x + radius/10, center.y});
  canvas.DrawLine({center.x - radius/10, center.y + radius / 10}, {center.x - radius/10, center.y});
  canvas.DrawLine({center.x - 2, center.y}, {center.x +2, center.y});

  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(-0.25)-roll, Angle::Degrees(0.25)-roll);
  
  // canvas.DrawAnnulus(center, radius-radius/20, radius, Angle::Degrees(9.9)-roll, Angle::Degrees(10.1)-roll);
  // canvas.DrawAnnulus(center, radius-radius/20, radius, Angle::Degrees(19.9)-roll, Angle::Degrees(20.1)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(29.9)-roll, Angle::Degrees(30.1)-roll);
  // canvas.DrawAnnulus(center, radius-radius/20, radius-radius/20+2, Angle::Degrees(44.9)-roll, Angle::Degrees(45.1)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(59.9)-roll, Angle::Degrees(60.1)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(89.9)-roll, Angle::Degrees(90.1)-roll);
  
  // canvas.DrawAnnulus(center, radius-radius/20, radius, Angle::Degrees(-10.1)-roll, Angle::Degrees(-9.9)-roll);
  // canvas.DrawAnnulus(center, radius-radius/20, radius, Angle::Degrees(-20.1)-roll, Angle::Degrees(-19.9)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(-30.1)-roll, Angle::Degrees(-29.9)-roll);
  // canvas.DrawAnnulus(center, radius-radius/20, radius-radius/20+2, Angle::Degrees(-45.1)-roll, Angle::Degrees(-44.9)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(-60.1)-roll, Angle::Degrees(-59.9)-roll);
  // canvas.DrawAnnulus(center, radius-radius/10, radius, Angle::Degrees(-90.1)-roll, Angle::Degrees(-89.9)-roll);



  // draw 45 degree dash marks
  canvas.Select(look.mark_pen);
  //  int rr2p = uround(radius * M_SQRT1_2) + Layout::Scale(1);
  //  int rr2n = rr2p - Layout::Scale(6);
  // canvas.DrawLine({center.x + rr2p, center.y - rr2p},
  //                 {center.x + rr2n, center.y - rr2n});
  // canvas.DrawLine({center.x - rr2p, center.y - rr2p},
  //                 {center.x - rr2n, center.y - rr2n});

  int x1 = - radius + Layout::Scale(2);
  int y1 = 0;
  int x2 = - radius + Layout::Scale(12);
  int y2 = 0;

  auto a = Angle::Degrees(0 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(30 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(60 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(90 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(120 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(150 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(180 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});

  x1 = - radius + Layout::Scale(2+5);
  a = Angle::Degrees(70 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(80 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(100 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(110 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});

  x2 = - radius + Layout::Scale(12-5+2);
  a = Angle::Degrees(45 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});
  a = Angle::Degrees(135 - bank_degrees);
  canvas.DrawLine({int(x1 * a.cos() - y1 * a.sin() + center.x), 
                   int(x1 * a.sin() + y1 * a.cos() + center.y)}, 
                  {int(x2 * a.cos() - y2 * a.sin() + center.x), 
                   int(x2 * a.sin() + y2 * a.cos() + center.y)});


  // rr2p = uround(radius * 0.866) + Layout::Scale(1);
  // rr2n = rr2p - Layout::Scale(6);
  // canvas.DrawLine({center.x + rr2p, center.y - rr2p},
  //                 {center.x + rr2n, center.y - rr2n});
  // canvas.DrawLine({center.x - rr2p, center.y - rr2p},
  //                 {center.x - rr2n, center.y - rr2n});
}
