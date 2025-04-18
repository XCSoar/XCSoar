// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PolylineDecoder.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Point2D.hpp"

#include <stdexcept>

using PointE5 = Point2D<int_fast32_t>;

[[gnu::const]]
static Angle
AngleFromE5(int_fast32_t i) noexcept
{
  return Angle::Degrees(i * 1e-5);
}

int_fast32_t
ReadPolylineInt(std::string_view &src)
{
  int_fast32_t value = 0;
  bool more;
  unsigned shift = 0;

  do {
    if (src.empty())
      throw std::invalid_argument{"Unexpected end of polyline string"};

    const char ch = src.front();
    src = src.substr(1);

    //fprintf(stderr, "%#x\n", ch);
    if (ch < 63 || ch > 63 + 0x3f)
      throw std::invalid_argument{"Bad character in polyline string"};

    int_fast32_t i = static_cast<int_fast32_t>(ch - 63);
    more = i & ~0x1f;

    value |= int_fast32_t(i & 0x1f) << shift;
    shift += 5;
  } while (more);

  /* if the lowest bit is set, then this is a negative value that was
     inverted */
  if (value & 0x1)
    value = ~value;

  /* discard the sign bit */
  value >>= 1;

  return value;
}

static PointE5
ReadPointE5(std::string_view &src)
{
  return {
    ReadPolylineInt(src),
    ReadPolylineInt(src),
  };
}

static GeoPoint
ToGeoPoint(PointE5 p)
{
  GeoPoint g{AngleFromE5(p.y), AngleFromE5(p.x)};
  if (!g.Check())
    throw std::invalid_argument{"Invalid coordinates"};

  return g;
}

GeoPoint
ReadPolylineGeoPoint(std::string_view &src)
{
  return ToGeoPoint(ReadPointE5(src));
}

GeoPoint
ReadPolylineLonLat(std::string_view &src)
{
  const auto p = ReadPointE5(src);
  GeoPoint g{AngleFromE5(p.x), AngleFromE5(p.y)};
  if (!g.Check())
    throw std::invalid_argument{"Invalid coordinates"};

  return g;
}

std::vector<GeoPoint>
DecodePolyline(std::string_view src)
{
  std::vector<GeoPoint> v;

  PointE5 last{};
  while (!src.empty())
    v.emplace_back(ToGeoPoint(last += ReadPointE5(src)));

  return v;
}
