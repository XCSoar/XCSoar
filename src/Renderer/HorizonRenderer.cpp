// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <algorithm>

#include "HorizonRenderer.hpp"
#include "Look/HorizonLook.hpp"
#include "Math/Constants.hpp"
#include "Math/Util.hpp"
#include "NMEA/Attitude.hpp"
#include "RadarRenderer.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/StringFormat.hpp"

#define DONT_INTERSECT 0
#define DO_INTERSECT 1
#define COLLINEAR 2

#define LEFT 0
#define TOP 1
#define RIGHT 2
#define BOTTOM 3
#define NONE 4

#define MAX_HEIGHT_DEGREES 40.f

void
HorizonRenderer::drawAircraftSymbol(Canvas &canvas, const PixelPoint &center,
                                    int radius, const HorizonLook &look)
{
  canvas.Select(look.aircraft_pen);
  canvas.Select(look.aircraft_brush);

  canvas.DrawLine({center.x + radius - radius / 5, center.y},
                  {center.x + radius / 10, center.y});
  canvas.DrawLine({center.x - radius + radius / 5, center.y},
                  {center.x - radius / 10, center.y});
  canvas.DrawLine({center.x + radius / 10, center.y + radius / 10},
                  {center.x + radius / 10, center.y});
  canvas.DrawLine({center.x - radius / 10, center.y + radius / 10},
                  {center.x - radius / 10, center.y});
  canvas.DrawCircle(center, 2);
}

int
HorizonRenderer::lines_intersect(PixelPoint p1, PixelPoint p2, PixelPoint p3,
                                 PixelPoint p4, PixelPoint &intersect)
{
  int64_t a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
  int64_t r1, r2, r3, r4;         /* 'Sign' values */
  int64_t denom, offset, num;     /* Intermediate values */

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
  if (r3 != 0 && r4 != 0 && r3 * r4 >= 0) return (DONT_INTERSECT);

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
  if (r1 != 0 && r2 != 0 && r1 * r2 >= 0) return (DONT_INTERSECT);

  /* Line segments intersect: compute intersection point.
   */
  denom = a1 * b2 - a2 * b1;
  if (denom == 0) return (COLLINEAR);
  offset = denom < 0 ? -denom / 2 : denom / 2;

  /* The denom/2 is to get rounding instead of truncating.  It
   * is added or subtracted to the numerator, depending upon the
   * sign of the numerator.
   */
  num = b1 * c2 - b2 * c1;
  intersect.x = (num < 0 ? num - offset : num + offset) / denom;

  num = a2 * c1 - a1 * c2;
  intersect.y = (num < 0 ? num - offset : num + offset) / denom;

  return (DO_INTERSECT);
}

void
HorizonRenderer::rotate(PixelPoint point, PixelPoint center, Angle a,
                        PixelPoint &rotated)
{
  rotated.x = int(point.x * a.cos() - point.y * a.sin() + center.x);
  rotated.y = int(point.x * a.sin() + point.y * a.cos() + center.y);
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

  RadarRenderer radar_renderer{Layout::Scale(1U)};
  radar_renderer.UpdateLayout(rc);

  const auto center = radar_renderer.GetCenter();
  const int radius = radar_renderer.GetRadius();

  auto bank_degrees = attitude.bank_angle_available
    ? attitude.bank_angle.Degrees()
    : 0.;

  auto pitch_degrees = attitude.pitch_angle_available
    ? attitude.pitch_angle.Degrees()
    : 0.;

  int w = static_cast<int>(rc.GetWidth());
  int h = static_cast<int>(rc.GetHeight());

  // compute theoretical horizon line
  PixelPoint p1 = {-w, int(h / 2.f * pitch_degrees / MAX_HEIGHT_DEGREES)};
  PixelPoint p2 = {w, int(h / 2.f * pitch_degrees / MAX_HEIGHT_DEGREES)};
  rotate(p1, center, Angle::Degrees(-bank_degrees), p1);
  rotate(p2, center, Angle::Degrees(-bank_degrees), p2);

  PixelPoint p3 = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};
  PixelPoint k11 = {0, 0}; // up left
  PixelPoint k12 = {w, 0}; // up right
  PixelPoint k22 = {w, h}; // down right
  PixelPoint k21 = {0, h}; // down left

  int i, j;
  BulkPixelPoint corners[] = {k21, k11, k12, k22, k21};
  PixelPoint intersect_left = {0, 0};
  PixelPoint intersect_right = {0, 0};
  for (i = 0; i < 4; i++)
  {
    int res =
        lines_intersect(p1, p3, corners[i], corners[i + 1], intersect_left);
    if (res == DO_INTERSECT) break;
  } // if no intersection, i == NONE at end of loop
  for (j = 0; j < 4; j++)
  {
    int res =
        lines_intersect(p3, p2, corners[j], corners[j + 1], intersect_right);
    if (res == DO_INTERSECT) break;
  } // if no intersection, j == NONE at end of loop

  bool invert = false;
  if (intersect_left.x > intersect_right.x)
  {
    PixelPoint tmp = intersect_left;
    intersect_left = intersect_right;
    intersect_right = tmp;
    int t = i;
    i = j;
    j = t;
    invert = true;
  }

  if (!invert)
  {
    canvas.Select(look.sky_pen);
    canvas.Select(look.sky_brush);
  }
  else
  {
    canvas.Select(look.terrain_pen);
    canvas.Select(look.terrain_brush);
  }
  if (i == LEFT)
  {
    if (j == TOP)
    {
      BulkPixelPoint sky[3] = {intersect_left, k11, intersect_right};
      canvas.DrawPolygon(sky, 3);
    }
    else if (j == RIGHT)
    {
      BulkPixelPoint sky[4] = {intersect_left, k11, k12, intersect_right};
      canvas.DrawPolygon(sky, 4);
    }
    else if (j == BOTTOM)
    {
      BulkPixelPoint sky[5] = {intersect_left, k11, k12, k22, intersect_right};
      canvas.DrawPolygon(sky, 5);
    }
  }
  else if (i == TOP)
  {
    if (j == RIGHT)
    {
      BulkPixelPoint sky[3] = {intersect_left, k12, intersect_right};
      canvas.DrawPolygon(sky, 3);
    }
    else if (j == BOTTOM)
    {
      BulkPixelPoint sky[4] = {intersect_left, k12, k22, intersect_right};
      canvas.DrawPolygon(sky, 4);
    }
  }
  else if (i == BOTTOM)
  {
    if (j == TOP)
    {
      BulkPixelPoint sky[4] = {intersect_left, k21, k11, intersect_right};
      canvas.DrawPolygon(sky, 4);
    }
    else if (j == RIGHT)
    {
      BulkPixelPoint sky[5] = {intersect_left, k21, k11, k12, intersect_right};
      canvas.DrawPolygon(sky, 5);
    }
  }

  if (!invert)
  {
    canvas.Select(look.terrain_pen);
    canvas.Select(look.terrain_brush);
  }
  else
  {
    canvas.Select(look.sky_pen);
    canvas.Select(look.sky_brush);
  }
  if (i == LEFT)
  {
    if (j == TOP)
    {
      BulkPixelPoint terrain[5] = {intersect_left, k21, k22, k12,
                                   intersect_right};
      canvas.DrawPolygon(terrain, 5);
    }
    else if (j == RIGHT)
    {
      BulkPixelPoint terrain[4] = {intersect_left, k21, k22, intersect_right};
      canvas.DrawPolygon(terrain, 4);
    }
    else if (j == BOTTOM)
    {
      BulkPixelPoint terrain[3] = {intersect_left, k21, intersect_right};
      canvas.DrawPolygon(terrain, 3);
    }
  }
  else if (i == TOP)
  {
    if (j == RIGHT)
    {
      BulkPixelPoint terrain[5] = {intersect_left, k11, k21, k22,
                                   intersect_right};
      canvas.DrawPolygon(terrain, 5);
    }
    else if (j == BOTTOM)
    {
      BulkPixelPoint terrain[4] = {intersect_left, k11, k21, intersect_right};
      canvas.DrawPolygon(terrain, 4);
    }
  }
  else if (i == BOTTOM)
  {
    if (j == TOP)
    {
      BulkPixelPoint terrain[4] = {intersect_left, k22, k12, intersect_right};
      canvas.DrawPolygon(terrain, 4);
    }
    else if (j == RIGHT)
    {
      BulkPixelPoint terrain[3] = {intersect_left, k22, intersect_right};
      canvas.DrawPolygon(terrain, 3);
    }
  }

  // draw horizon line
  if (i != NONE && j != NONE)
  {
    canvas.Select(look.horizon_pen);
    canvas.DrawLine(intersect_left, intersect_right);
  }
  else
  { // draw full terrain/sky
    if (pitch_degrees < 0)
    {
      canvas.Select(look.terrain_pen);
      canvas.Select(look.terrain_brush);
    }
    else
    {
      canvas.Select(look.sky_pen);
      canvas.Select(look.sky_brush);
    }
    BulkPixelPoint square[4] = {k21, k11, k12, k22};
    canvas.DrawPolygon(square, 4);
  }

  // marks
  canvas.Select(look.mark_pen);
  canvas.Select(look.mark_brush);

  PixelPoint m1 = {-radius + Layout::Scale(2), 0};
  PixelPoint m2 = {-radius + Layout::Scale(12), 0};

  for (int angle : {0, 30, 60, 120, 150, 180})
  {
    rotate(m1, center, Angle::Degrees(angle - bank_degrees), p1);
    rotate(m2, center, Angle::Degrees(angle - bank_degrees), p2);
    canvas.DrawLine(p1, p2);
  }

  m1.x = -radius + Layout::Scale(7);

  for (int angle : {70, 80, 100, 110})
  {
    rotate(m1, center, Angle::Degrees(angle - bank_degrees), p1);
    rotate(m2, center, Angle::Degrees(angle - bank_degrees), p2);
    canvas.DrawLine(p1, p2);
  }

  m2.x = -radius + Layout::Scale(9);

  for (int angle : {45, 135})
  {
    rotate(m2, center, Angle::Degrees(angle - bank_degrees), p2);
    canvas.DrawCircle(p2, 2);
  }

  PixelPoint m3 = {-radius + Layout::Scale(12), 0};
  m1 = {-radius + Layout::Scale(2), -5};
  m2 = {-radius + Layout::Scale(2), 5};
  rotate(m1, center, Angle::Degrees(90 - bank_degrees), p1);
  rotate(m2, center, Angle::Degrees(90 - bank_degrees), p2);
  rotate(m3, center, Angle::Degrees(90 - bank_degrees), p3);

  BulkPixelPoint zero_mark[3] = {p1, p2, p3};
  canvas.DrawPolygon(zero_mark, 3);

  // pitch marks
  int pitch_10 = (int(pitch_degrees) / 10) * 10;
  for (int k = pitch_10 - 20; k <= pitch_10 + 20; k += 10)
  {
    if (k == 0) continue;
    m1 = {-radius / 5, static_cast<int>(h / 2.f * (pitch_degrees - k) /
                                        MAX_HEIGHT_DEGREES)};
    m2 = {+radius / 5, static_cast<int>(h / 2.f * (pitch_degrees - k) /
                                        MAX_HEIGHT_DEGREES)};
    rotate(m1, center, Angle::Degrees(-bank_degrees), p1);
    rotate(m2, center, Angle::Degrees(-bank_degrees), p2);
    canvas.DrawLine(p1, p2);

    // stop showing text numbers when in infobox mode
    if (radius > 100)
    {
      canvas.Select(look.mark_font);
      canvas.SetBackgroundTransparent();
      canvas.SetTextColor(COLOR_WHITE);
      TCHAR buffer[5];
      StringFormatUnsafe(buffer, _T("%+3d"), k);
      PixelSize ts = canvas.CalcTextSize(buffer);
      m2 = {+radius / 5 + static_cast<int>(ts.height / 2),
            static_cast<int>(h / 2.f * (pitch_degrees - k) /
                             MAX_HEIGHT_DEGREES) -
                static_cast<int>(ts.height / 2)};
      rotate(m2, center, Angle::Degrees(-bank_degrees), p2);
      canvas.DrawText(p2, buffer);
    }
    for (int k = pitch_10 - 25; k <= pitch_10 + 25; k += 10)
    {
      m1 = {-radius / 10, static_cast<int>(h / 2.f * (pitch_degrees - k) /
                                           MAX_HEIGHT_DEGREES)};
      m2 = {+radius / 10, static_cast<int>(h / 2.f * (pitch_degrees - k) /
                                           MAX_HEIGHT_DEGREES)};
      rotate(m1, center, Angle::Degrees(-bank_degrees), p1);
      rotate(m2, center, Angle::Degrees(-bank_degrees), p2);
      canvas.DrawLine(p1, p2);
    }
  }

  HorizonRenderer::drawAircraftSymbol(canvas, center, radius, look);

  if (radius > 100)
  {
    BulkPixelPoint triangle[3] = {
        {center.x - Layout::Scale(4), center.y - radius + Layout::Scale(22)},
        {center.x, center.y - radius + Layout::Scale(12)},
        {center.x + Layout::Scale(4), center.y - radius + Layout::Scale(22)}};
    canvas.DrawPolygon(triangle, 3);
  }
}
