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

#include "Geo/GeoClip.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static inline GeoPoint
make_geo_point(int longitude, int latitude)
{
  return GeoPoint(Angle::Degrees(longitude),
                  Angle::Degrees(latitude));
}

static void
test_ClipLine(const GeoClip &clip, GeoPoint a, GeoPoint b,
               const GeoPoint a2, const GeoPoint b2)
{
  clip.ClipLine(a, b);
  ok1(equals(a, a2));
  ok1(equals(b, b2));
}

static void
test_clip_line()
{
  GeoClip clip(GeoBounds(make_geo_point(2, 5), make_geo_point(6, 1)));

  /* no clipping */
  test_ClipLine(clip, make_geo_point(2, 5), make_geo_point(6, 1),
                make_geo_point(2, 5), make_geo_point(6, 1));

  /* clipping at east border */
  test_ClipLine(clip, make_geo_point(2, 4), make_geo_point(7, 4),
                make_geo_point(2, 4), make_geo_point(6, 4));

  /* clipping east & west */
  test_ClipLine(clip, make_geo_point(1, 4), make_geo_point(7, 4),
                make_geo_point(2, 4), make_geo_point(6, 4));

  /* clipping north */
  test_ClipLine(clip, make_geo_point(3, 7), make_geo_point(3, 2),
                make_geo_point(3, 5), make_geo_point(3, 2));

  /* clipping north & south */
  test_ClipLine(clip, make_geo_point(3, 7), make_geo_point(3, -1000),
                make_geo_point(3, 5), make_geo_point(3, 1));

  /* clipping southwest */
  test_ClipLine(clip, make_geo_point(5, 2), make_geo_point(7, 0),
                make_geo_point(5, 2), make_geo_point(6, 1));

  /* clipping northwest & southeast */
  test_ClipLine(clip, make_geo_point(0, 9), make_geo_point(9, -3),
                make_geo_point(3, 5), make_geo_point(6, 1));
}

static int
find(const GeoPoint *haystack, unsigned size, const GeoPoint needle)
{
  for (unsigned i = 0; i < size; ++i)
    if (equals(haystack[i], needle))
      return i;
  return -1;
}

static bool
equals(const GeoPoint *a, unsigned a_size, const GeoPoint *b, unsigned b_size)
{
  if (a_size != b_size)
    return false;

  if (a_size == 0)
    return true;

  int offset = find(b, b_size, *a);
  if (offset < 0)
    return false;

  for (unsigned i = 0; i < a_size; ++i) {
    unsigned j = (i + offset) % a_size;
    if (!equals(a[i], b[j]))
      return false;
  }

  return true;
}

static void
test_clip_polygon(const GeoClip &clip, const GeoPoint *src, unsigned src_size,
                  const GeoPoint *result, unsigned result_size)
{
  GeoPoint dest[64];
  unsigned dest_size = clip.ClipPolygon(dest, src, src_size);
  ok1(equals(result, result_size, dest, dest_size));
}

static void
test_clip_polygon()
{
  GeoClip clip(GeoBounds(make_geo_point(2, 5), make_geo_point(6, 1)));

  /* invalid polygon */
  const GeoPoint src1[2] = {
    make_geo_point(0, 0),
    make_geo_point(1, 1),
  };

  test_clip_polygon(clip, src1, 0, NULL, 0);
  test_clip_polygon(clip, src1, 1, NULL, 0);
  test_clip_polygon(clip, src1, 2, NULL, 0);

  /* no clipping */
  const GeoPoint src2[3] = {
    make_geo_point(3, 4),
    make_geo_point(5, 4),
    make_geo_point(3, 2),
  };
  test_clip_polygon(clip, src2, 3, src2, 3);

  /* one vertex clipped */
  const GeoPoint src3[3] = {
    make_geo_point(3, 4),
    make_geo_point(9, 4),
    make_geo_point(3, 2),
  };
  const GeoPoint result3[4] = {
    make_geo_point(3, 4),
    make_geo_point(6, 4),
    make_geo_point(6, 3),
    make_geo_point(3, 2),
  };
  test_clip_polygon(clip, src3, 3, result3, 4);

  /* two vertices clipped */
  const GeoPoint src4[3] = {
    make_geo_point(1, 4),
    make_geo_point(9, 4),
    make_geo_point(3, 2),
  };
  const GeoPoint result4[5] = {
    make_geo_point(2, 3),
    make_geo_point(2, 4),
    make_geo_point(6, 4),
    make_geo_point(6, 3),
    make_geo_point(3, 2),
  };
  test_clip_polygon(clip, src4, 3, result4, 5);

  /* clipping a secant */
  const GeoPoint src5[3] = {
    make_geo_point(1, 2),
    make_geo_point(3, 2),
    make_geo_point(3, 0),
  };
  const GeoPoint result5[4] = {
    make_geo_point(2, 1),
    make_geo_point(2, 2),
    make_geo_point(3, 2),
    make_geo_point(3, 1),
  };
  test_clip_polygon(clip, src5, 3, result5, 4);

  /* all four vertices clipped */
  const GeoPoint src6[4] = {
    make_geo_point(1, 3),
    make_geo_point(4, 6),
    make_geo_point(7, 3),
    make_geo_point(4, 0),
  };
  const GeoPoint result6[8] = {
    make_geo_point(2, 4),
    make_geo_point(3, 5),
    make_geo_point(5, 5),
    make_geo_point(6, 4),
    make_geo_point(6, 2),
    make_geo_point(5, 1),
    make_geo_point(3, 1),
    make_geo_point(2, 2),
  };
  test_clip_polygon(clip, src6, 4, result6, 8);

  /* rectangle full clip */
  const GeoPoint src7[4] = {
    make_geo_point(-10, -10),
    make_geo_point(-10, 10),
    make_geo_point(10, 10),
    make_geo_point(10, -10),
  };
  const GeoPoint result7[4] = {
    make_geo_point(2, 1),
    make_geo_point(2, 5),
    make_geo_point(6, 5),
    make_geo_point(6, 1),
  };
  test_clip_polygon(clip, src7, 4, result7, 4);

  /* triangle full clip */
  const GeoPoint src8[3] = {
    make_geo_point(-10, 50),
    make_geo_point(50, 5),
    make_geo_point(-5, -50),
  };
  test_clip_polygon(clip, src8, 3, result7, 4);
}

int main(int argc, char **argv)
{
  plan_tests(24);

  test_clip_line();
  test_clip_polygon();

  return exit_status();
}
