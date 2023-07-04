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

#define	DONT_INTERSECT    0
#define	DO_INTERSECT      1
#define COLLINEAR         2

#define LEFT 0
#define TOP 1
#define RIGHT 2
#define BOTTOM 3

int HorizonRenderer::lines_intersect(PixelPoint p1, PixelPoint p2, 
                    PixelPoint p3, PixelPoint p4, 
                    PixelPoint &intersect){
    long a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
    long r1, r2, r3, r4;         /* 'Sign' values */
    long denom, offset, num;     /* Intermediate values */

    /* Compute a1, b1, c1, where line joining points 1 and 2
     * is "a1 x  +  b1 y  +  c1  =  0".
     */
    a1 = p2.y - p1.y;
    b1 = p1.x - p2.x;
    c1 = p2.x * p1.y - p1.x * p2.y;

    /* Compute r3 and r4.
     */
    r3 = a1 * p3.x + b1 * p3.y + c1;
    r4 = a1 * p4.x + b1 * p4.y + c1;

    /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
     * same side of line 1, the line segments do not intersect.
     */
    if ( r3 != 0 && r4 != 0 && r3 * r4 >=0)
        return ( DONT_INTERSECT );

    /* Compute a2, b2, c2 */
    a2 = p4.y - p3.y;
    b2 = p3.x - p4.x;
    c2 = p4.x * p3.y - p3.x * p4.y;

    /* Compute r1 and r2 */
    r1 = a2 * p1.x + b2 * p1.y + c2;
    r2 = a2 * p2.x + b2 * p2.y + c2;

    /* Check signs of r1 and r2.  If both point 1 and point 2 lie
     * on same side of second line segment, the line segments do
     * not intersect.
     */
    if ( r1 != 0 && r2 != 0 && r1 * r2 >= 0)
        return ( DONT_INTERSECT );

    /* Line segments intersect: compute intersection point. 
     */
    denom = a1 * b2 - a2 * b1;
    if ( denom == 0 )
        return ( COLLINEAR );
    offset = denom < 0 ? - denom / 2 : denom / 2;

    /* The denom/2 is to get rounding instead of truncating.  It
     * is added or subtracted to the numerator, depending upon the
     * sign of the numerator.
     */
    num = b1 * c2 - b2 * c1;
    intersect.x = ( num < 0 ? num - offset : num + offset ) / denom;

    num = a2 * c1 - a1 * c2;
    intersect.y = ( num < 0 ? num - offset : num + offset ) / denom;

    return ( DO_INTERSECT );
}


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
    // canvas.DrawSegment(center, radius*3./2, alpha2, alpha1, true);
    // canvas.Select(look.horizon_pen);
    canvas.DrawSegment(center, radius, alpha2, alpha1, true);
    // canvas.DrawSegment(center, radius - Layout::Scale(12), alpha2, alpha1, true);
  
  }

  // draw ground part
  if (cosine_ratio < 1) { // when greater than 1 then the ground is not visible
    canvas.Select(look.terrain_pen);
    canvas.Select(look.terrain_brush);
    // canvas.DrawSegment(center, radius*2, alpha1, alpha2, true);
    // canvas.Select(look.horizon_pen);
    canvas.DrawSegment(center, radius, alpha1, alpha2, true);
  }

  // draw horizon line
  canvas.Select(look.horizon_pen);
  auto roll = -Angle::Degrees(bank_degrees);
  auto pitch = Angle::Degrees(pitch_degrees);

  double u_y = radius * roll.tan();
  double u_p = radius * pitch.tan();

  PixelPoint c = center;
  PixelPoint p1 = {0, c.y - int(u_y - u_p)};
  PixelPoint p2 = {2*c.x, c.y + int(u_y + u_p)};
  // canvas.DrawLine(p1, p2);

  // std::cout << "p1: {" << p1.x << ", " << p1.y << "}, "
  //   << ", p2: {" << p2.x << ", " << p2.y << "} "
  //   << std::endl;
  // std::cout << "w: " << rc.GetWidth()
  //   << ", h: " << rc.GetHeight()
  //   << std::endl;

  int w = int(rc.GetWidth());
  int h = int(rc.GetHeight());
  PixelPoint k11 = {0,0};
  PixelPoint k12 = {w,0};
  PixelPoint k22 = {w,h};
  PixelPoint k21 = {0,h};

  BulkPixelPoint terrain[] = {k11, k12, k21, k22};
  canvas.Select(look.terrain_pen);
  canvas.Select(look.terrain_brush);
  canvas.DrawPolygon(terrain, 4);

  int i, j;
  BulkPixelPoint corners[] = {k21, k11, k12, k22, k21};
  PixelPoint intersect_left = {0, 0};
  PixelPoint intersect_right = {0, 0};
  for (i=0; i < 4; i++){
    int res = lines_intersect(p1, center, corners[i], 
                              corners[i+1], intersect_left);
    if (res == DO_INTERSECT) break;
    if (i==3) return;
  }
  for (j=0; j < 4; j++){
    int res = lines_intersect(center, p2, corners[j], 
                              corners[j+1], intersect_right);
    if (res == DO_INTERSECT) break;
    if (j==3) return;
  }
  // std::cout << "i: " << i << ", {" << intersect_left.x << ", " << intersect_left.y << "}, "
  //   << ", j: " << j << ", {" << intersect_right.x << ", " << intersect_right.y << "}, "
  //   << std::endl;

  canvas.Select(look.sky_pen);
  canvas.Select(look.sky_brush);
  if (i==LEFT){
    if (j==TOP) {
      BulkPixelPoint sky[3] = {intersect_left, k11, intersect_right};
      canvas.DrawPolygon(sky, 3);
    } else if (j==RIGHT){
      BulkPixelPoint sky[4] = {intersect_left, k11, k12, intersect_right};
      canvas.DrawPolygon(sky, 4);
    } else if (j==BOTTOM) {
      BulkPixelPoint sky[5] = {intersect_left, k11, k12, k22, intersect_right};
      canvas.DrawPolygon(sky, 5);
    }
  }
  else if (i==TOP){
    if (j==RIGHT) {
      BulkPixelPoint sky[3] = {intersect_left, k12, intersect_right};
      canvas.DrawPolygon(sky, 3);
    } else if (j==BOTTOM) {
      BulkPixelPoint sky[4] = {intersect_left, k12, k22, intersect_right};
      canvas.DrawPolygon(sky, 4);
    }
  }
  else if (i==BOTTOM){
    if (j==TOP) {
      BulkPixelPoint sky[4] = {intersect_left, k21, k11, intersect_right};
      canvas.DrawPolygon(sky, 4);
    } else if (j==RIGHT) {
      BulkPixelPoint sky[5] = {intersect_left, k21, k11, k12, intersect_right};
      canvas.DrawPolygon(sky, 5);
    }
  }

  canvas.Select(look.horizon_pen);
  canvas.DrawLine(p1, p2);


  
  // draw aircraft symbol
  canvas.Select(look.aircraft_pen);
  canvas.DrawLine({center.x + radius / 2, center.y}, {center.x + radius / 10, center.y});
  canvas.DrawLine({center.x - radius / 2, center.y}, {center.x - radius / 10, center.y});
  canvas.DrawLine({center.x + radius/10, center.y + radius / 10}, {center.x + radius/10, center.y});
  canvas.DrawLine({center.x - radius/10, center.y + radius / 10}, {center.x - radius/10, center.y});
  canvas.DrawLine({center.x - 2, center.y}, {center.x +2, center.y});

  BulkPixelPoint triangle[3] = {
    {center.x - radius + Layout::Scale(22), center.y-Layout::Scale(5)},
    {center.x - radius + Layout::Scale(12), center.y},
    {center.x - radius + Layout::Scale(22), center.y+Layout::Scale(5)}};
  canvas.DrawPolygon(triangle, 3);

  /*
  // draw 45 degree dash marks
  canvas.Select(look.mark_pen);

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

*/
}
