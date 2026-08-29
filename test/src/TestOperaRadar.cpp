// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/OPERA/Radar.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include <string.h>

/**
 * Assembles a classic little endian TIFF header, so that the
 * directory reader can be exercised without the network and without a
 * five megabyte fixture in the tree.
 */
class HeaderBuilder {
  std::vector<uint8_t> data;

public:
  HeaderBuilder() {
    data = {'I', 'I', 42, 0, 0, 0, 0, 0};
  }

  std::size_t Size() const noexcept { return data.size(); }

  void PutU16(std::size_t at, uint16_t value) noexcept {
    data[at] = uint8_t(value);
    data[at + 1] = uint8_t(value >> 8);
  }

  void PutU32(std::size_t at, uint32_t value) noexcept {
    for (unsigned i = 0; i < 4; ++i)
      data[at + i] = uint8_t(value >> (8 * i));
  }

  std::size_t AppendU32Array(const std::vector<uint32_t> &values) {
    const auto at = data.size();
    for (const auto value : values) {
      data.resize(data.size() + 4);
      PutU32(data.size() - 4, value);
    }

    return at;
  }

  std::size_t AppendDoubles(const std::vector<double> &values) {
    const auto at = data.size();
    for (const auto value : values) {
      uint64_t bits;
      std::memcpy(&bits, &value, sizeof(bits));
      data.resize(data.size() + 8);
      for (unsigned i = 0; i < 8; ++i)
        data[data.size() - 8 + i] = uint8_t(bits >> (8 * i));
    }

    return at;
  }

  /** @return the offset the directory was written at */
  std::size_t AppendDirectory(const std::vector<std::array<uint32_t, 4>> &tags) {
    const auto at = data.size();
    data.resize(at + 2 + tags.size() * 12 + 4, 0);
    PutU16(at, uint16_t(tags.size()));

    std::size_t e = at + 2;
    for (const auto &tag : tags) {
      PutU16(e, uint16_t(tag[0]));
      PutU16(e + 2, uint16_t(tag[1]));
      PutU32(e + 4, tag[2]);
      PutU32(e + 8, tag[3]);
      e += 12;
    }

    return at;
  }

  void SetFirstDirectory(uint32_t at) noexcept { PutU32(4, at); }

  void SetNextDirectory(std::size_t directory, uint32_t next) noexcept {
    const auto n = uint16_t(data[directory] | (data[directory + 1] << 8));
    PutU32(directory + 2 + std::size_t(n) * 12, next);
  }

  std::span<const std::byte> Get() const noexcept {
    return std::as_bytes(std::span{data});
  }
};

/* the tag numbers the reader looks for */
static constexpr uint32_t TAG_WIDTH = 256, TAG_HEIGHT = 257;
static constexpr uint32_t TAG_COMPRESSION = 259, TAG_SAMPLES = 277;
static constexpr uint32_t TAG_TILE_WIDTH = 322, TAG_TILE_HEIGHT = 323;
static constexpr uint32_t TAG_TILE_OFFSETS = 324, TAG_TILE_LENGTHS = 325;
static constexpr uint32_t TAG_PIXEL_SCALE = 33550, TAG_TIEPOINT = 33922;
static constexpr uint32_t TYPE_SHORT = 3, TYPE_LONG = 4, TYPE_DOUBLE = 12;

/**
 * A two level composite whose full resolution grid is 1024 by 1024 in
 * 512 pixel tiles, arranged so that the projection centre falls in the
 * middle of it.
 */
static HeaderBuilder
MakeComposite()
{
  /* the grid origin, chosen so that 10E 55N lands at column 768, row
     768: the middle of the last of the four tiles, well clear of the
     boundaries so that the test asks about tile arithmetic rather
     than about which side of an edge a rounding error falls on */
  static constexpr double ORIGIN_X = 1950000 - 768 * 1000.0;
  static constexpr double ORIGIN_Y = -2100000 + 768 * 1000.0;

  HeaderBuilder b;

  const auto offsets0 = b.AppendU32Array({4096, 8192, 12288, 16384});
  const auto lengths0 = b.AppendU32Array({100, 200, 300, 400});
  const auto offsets1 = b.AppendU32Array({20480});
  const auto lengths1 = b.AppendU32Array({500});
  const auto scale = b.AppendDoubles({1000, 1000, 0});
  const auto tiepoint = b.AppendDoubles({0, 0, 0, ORIGIN_X, ORIGIN_Y, 0});

  const auto ifd0 = b.AppendDirectory({{
    {TAG_WIDTH, TYPE_LONG, 1, 1024},
    {TAG_HEIGHT, TYPE_LONG, 1, 1024},
    {TAG_COMPRESSION, TYPE_SHORT, 1, 8},
    {TAG_SAMPLES, TYPE_SHORT, 1, 2},
    {TAG_TILE_WIDTH, TYPE_SHORT, 1, 512},
    {TAG_TILE_HEIGHT, TYPE_SHORT, 1, 512},
    {TAG_TILE_OFFSETS, TYPE_LONG, 4, uint32_t(offsets0)},
    {TAG_TILE_LENGTHS, TYPE_LONG, 4, uint32_t(lengths0)},
    {TAG_PIXEL_SCALE, TYPE_DOUBLE, 3, uint32_t(scale)},
    {TAG_TIEPOINT, TYPE_DOUBLE, 6, uint32_t(tiepoint)},
  }});

  /* the overview carries no georeference of its own, exactly as the
     real file's do; its scale has to follow from the size ratio */
  const auto ifd1 = b.AppendDirectory({{
    {TAG_WIDTH, TYPE_LONG, 1, 512},
    {TAG_HEIGHT, TYPE_LONG, 1, 512},
    {TAG_COMPRESSION, TYPE_SHORT, 1, 8},
    {TAG_SAMPLES, TYPE_SHORT, 1, 2},
    {TAG_TILE_WIDTH, TYPE_SHORT, 1, 512},
    {TAG_TILE_HEIGHT, TYPE_SHORT, 1, 512},
    {TAG_TILE_OFFSETS, TYPE_LONG, 1, uint32_t(offsets1)},
    {TAG_TILE_LENGTHS, TYPE_LONG, 1, uint32_t(lengths1)},
  }});

  b.SetFirstDirectory(uint32_t(ifd0));
  b.SetNextDirectory(ifd0, uint32_t(ifd1));

  return b;
}

[[gnu::pure]]
static bool
Contains(const std::string &haystack, const char *needle) noexcept
{
  return haystack.find(needle) != std::string::npos;
}

int main()
{
  plan_tests(83);

  /* 12:07:30 minus the seven minute dissemination margin is 12:00:30,
     which rounds down to the 12:00 composite */
  const BrokenDateTime noon{BrokenDate{2026, 8, 28}, BrokenTime{12, 7, 30}};
  const auto url = OPERA::MakeCompositeURL(noon);

  ok1(Contains(url, "https://s3.waw3-1.cloudferro.com/openradar-24h/"));
  ok1(Contains(url, "/2026/08/28/OPERA/COMP/"));
  ok1(Contains(url, "OPERA@20260828T1200@0@DBZH.tiff"));

  /* the margin may cross midnight, and then the date has to follow */
  const BrokenDateTime after_midnight{BrokenDate{2026, 8, 28},
                                      BrokenTime{0, 3, 0}};
  const auto wrapped = OPERA::MakeCompositeURL(after_midnight);
  ok1(Contains(wrapped, "/2026/08/27/OPERA/COMP/"));
  ok1(Contains(wrapped, "OPERA@20260827T2355@0@DBZH.tiff"));

  /* without a clock we must not guess */
  ok1(OPERA::MakeCompositeURL(BrokenDateTime::Invalid()).empty());

  /* CompositeTime() must name the very frame the URL points at, or
     the age of the picture on the map would be computed against the
     wrong instant */
  const auto composite = OPERA::CompositeTime(noon);
  ok1(composite.hour == 12 && composite.minute == 0);
  ok1(composite.second == 0);
  ok1(!OPERA::CompositeTime(BrokenDateTime::Invalid()).IsPlausible());

  /* the frame we request is never newer than the margin and never
     older than the margin plus one cadence step; the age limit has to
     sit clear of that, or a frame would expire as it arrived */
  const auto fresh_age = noon - composite;
  ok1(fresh_age >= std::chrono::minutes{OPERA::LATENCY_MINUTES});
  ok1(fresh_age < std::chrono::minutes{OPERA::LATENCY_MINUTES +
                                       OPERA::CADENCE_MINUTES});
  ok1(fresh_age < std::chrono::minutes{OPERA::MAX_AGE_MINUTES});

  /* the projection centre lands on the grid's false origin */
  const auto centre = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                              Angle::Degrees(55)});
  ok1(equals(centre.x, 1950000));
  ok1(equals(centre.y, -2100000));

  /* east is +x, north is +y */
  const auto east = OPERA::Project(GeoPoint{Angle::Degrees(15),
                                            Angle::Degrees(55)});
  const auto north = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                             Angle::Degrees(60)});
  ok1(east.x > centre.x);
  ok1(std::fabs(east.y - centre.y) < std::fabs(east.x - centre.x));
  ok1(north.y > centre.y);
  ok1(std::fabs(north.x - centre.x) < std::fabs(north.y - centre.y));

  /* one degree of latitude is roughly 111km in this projection */
  const auto one_degree = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                                  Angle::Degrees(56)});
  ok1(std::fabs(one_degree.y - centre.y) > 110000);
  ok1(std::fabs(one_degree.y - centre.y) < 112000);

  /* weak echo is not drawn at all; the reference product clips it */
  ok1(OPERA::ClassifyReflectivity(0) < 0);
  ok1(OPERA::ClassifyReflectivity(18) < 0);

  /* and neither is "no radar here", which the composite marks with a
     large negative number */
  ok1(OPERA::ClassifyReflectivity(-9999000) < 0);

  /* its other marker is a NaN.  A literal one would be folded away
     under -ffast-math before it ever reached the function, so build
     it from the bit pattern -- which is what IsNotANumber() inspects
     anyway, for the same reason. */
  const uint64_t nan_bits = 0x7ff8000000000000ULL;
  double quiet_nan;
  std::memcpy(&quiet_nan, &nan_bits, sizeof(quiet_nan));

  ok1(OPERA::IsNotANumber(quiet_nan));
  ok1(OPERA::ClassifyReflectivity(quiet_nan) < 0);

  /* and an ordinary number must not be mistaken for one */
  ok1(!OPERA::IsNotANumber(30.0));
  ok1(!OPERA::IsNotANumber(-9999000.0));

  /* and the classes climb from there */
  ok1(OPERA::ClassifyReflectivity(19) == 0);
  ok1(OPERA::ClassifyReflectivity(40) > OPERA::ClassifyReflectivity(30));
  ok1(OPERA::GetClassColour(999) ==
      OPERA::GetClassColour(OPERA::N_CLASSES - 1));

  /* every class must be reachable, and every colour therefore
     reachable with it: two equal bounds would make the classifier step
     over one and that colour would never be drawn.  Walk the whole
     scale and check that it visits each index in turn, once. */
  int previous = -1;
  unsigned distinct = 0;
  for (double dbz = 0; dbz < 80; dbz += 0.01) {
    const int i = OPERA::ClassifyReflectivity(dbz);
    if (i == previous)
      continue;

    /* it may never skip an index or go backwards */
    if (i != previous + 1)
      break;

    previous = i;
    ++distinct;
  }

  ok1(distinct == OPERA::N_CLASSES);
  ok1(previous == int(OPERA::N_CLASSES) - 1);

  /* --- the display grid ------------------------------------------ */

  const GeoPoint augsburg{Angle::Degrees(10.9), Angle::Degrees(48.35)};

  /* the grid only exists between the two zoom bounds */
  ok1(!OPERA::GetAircraftTile(augsburg, OPERA::MIN_TILE_ZOOM - 1).IsValid());
  ok1(!OPERA::GetAircraftTile(augsburg, OPERA::MAX_TILE_ZOOM + 1).IsValid());
  ok1(!OPERA::GetAircraftTile(GeoPoint::Invalid(),
                              OPERA::MAX_TILE_ZOOM).IsValid());

  const auto base = OPERA::GetAircraftTile(augsburg, 9);
  ok1(base.IsValid());
  ok1(base.zoom == 9);

  /* the tile the aircraft is in must be the one that contains it */
  ok1(GeoBitmap::GetBounds(base).IsInside(augsburg));

  /* the grid must survive a fix at its edges: longitude 180 lands one
     tile past the last column and latitude past the Mercator cut-off
     runs to infinity, and casting either out-of-range double to
     uint32_t is undefined.  Both have to come back inside the grid. */
  for (const double longitude : {180.0, -180.0, 179.9999999}) {
    const auto edge = OPERA::GetAircraftTile(
      GeoPoint{Angle::Degrees(longitude), Angle::Degrees(48.35)}, 9);
    ok1(edge.IsValid() && edge.x < (uint32_t(1) << 9));
  }

  for (const double latitude : {89.9, -89.9, 85.05112878}) {
    const auto edge = OPERA::GetAircraftTile(
      GeoPoint{Angle::Degrees(10.9), Angle::Degrees(latitude)}, 9);
    ok1(edge.IsValid() && edge.y < (uint32_t(1) << 9));
  }

  const auto block = OPERA::CollectTiles(base);
  ok1(block.size() == OPERA::TILE_COUNT);

  /* nearest first: the aircraft's own tile leads, and the four tiles
     sharing an edge with it come before the four diagonal ones */
  ok1(OPERA::IsSameTile(block.front(), base));

  const auto ring = [&base](const GeoBitmap::TileData &t) {
    const int dx = int(t.x) - int(base.x), dy = int(t.y) - int(base.y);
    return dx * dx + dy * dy;
  };

  bool ordered = true;
  for (std::size_t i = 1; i < block.size(); ++i)
    if (ring(block[i]) < ring(block[i - 1]))
      ordered = false;

  ok1(ordered);
  ok1(ring(block[1]) == 1 && ring(block[4]) == 1);
  ok1(ring(block[5]) == 2);

  /* every tile of the block is distinct, or a slot would be spent
     twice on the same picture */
  auto sorted = block;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto &a, const auto &b){
              return a.x != b.x ? a.x < b.x : a.y < b.y;
            });
  ok1(std::adjacent_find(sorted.begin(), sorted.end(),
                         OPERA::IsSameTile) == sorted.end());

  /* an invalid centre yields no block at all */
  ok1(OPERA::CollectTiles({}).empty());

  /* a coarser tile covers more ground per pixel */
  const auto fine = GeoBitmap::GetBounds(OPERA::GetAircraftTile(augsburg, 10));
  const auto coarse = GeoBitmap::GetBounds(OPERA::GetAircraftTile(augsburg, 8));
  const auto fine_scale = OPERA::TileMetresPerPixel(fine, OPERA::TILE_PIXELS);
  const auto coarse_scale = OPERA::TileMetresPerPixel(coarse,
                                                      OPERA::TILE_PIXELS);
  ok1(fine_scale > 0);
  ok1(coarse_scale > 3 * fine_scale);
  ok1(OPERA::TileMetresPerPixel(fine, 0) == 0);

  /* --- the raster projector -------------------------------------- */

  /* it has to agree with Project() to the last bit, being nothing but
     the same arithmetic with the trigonometry hoisted out */
  static constexpr unsigned WIDTH = 8;
  const double left = 8.0, right = 12.0, latitude = 50.0;
  const OPERA::RasterProjector projector{left, right, WIDTH};
  ok1(projector.GetWidth() == WIDTH);

  DoublePoint2D row[WIDTH];
  projector.ProjectRow(latitude, row);

  bool agrees = true;
  for (unsigned x = 0; x < WIDTH; ++x) {
    const double longitude = left + (right - left) * (x + 0.5) / WIDTH;
    const auto expected = OPERA::Project(GeoPoint{Angle::Degrees(longitude),
                                                  Angle::Degrees(latitude)});
    if (std::fabs(row[x].x - expected.x) > 1e-6 ||
        std::fabs(row[x].y - expected.y) > 1e-6)
      agrees = false;
  }

  ok1(agrees);

  /* --- the composite directory ----------------------------------- */

  const auto composite_header = MakeComposite();
  const auto index = OPERA::ParseIndex(composite_header.Get());

  ok1(index.IsValid());
  ok1(index.levels.size() == 2);
  ok1(index.levels[0].width == 1024 && index.levels[0].height == 1024);
  ok1(index.levels[0].TilesAcross() == 2 && index.levels[0].TilesDown() == 2);
  ok1(index.levels[0].tile_offset.size() == 4);
  ok1(index.levels[0].tile_length[3] == 400);
  ok1(equals(index.levels[0].scale, 1000));

  /* the overview has no georeference of its own; it must inherit the
     origin and take its scale from the size ratio, or every pixel of
     it would land in the wrong place */
  ok1(equals(index.levels[1].scale, 2000));
  ok1(equals(index.levels[1].origin_x, index.levels[0].origin_x));
  ok1(equals(index.levels[1].origin_y, index.levels[0].origin_y));
  ok1(index.levels[1].IsUsable());

  /* garbage must be refused rather than parsed into nonsense */
  const std::byte truncated[4]{};
  try {
    OPERA::ParseIndex(std::span{truncated});
    ok1(false);
  } catch (...) {
    ok1(true);
  }

  const std::byte not_tiff[16]{std::byte{'X'}, std::byte{'Y'}};
  try {
    OPERA::ParseIndex(std::span{not_tiff});
    ok1(false);
  } catch (...) {
    ok1(true);
  }

  /* a directory that reaches past what was fetched must be refused
     rather than read off the end.  The offsets in the file are 32 bit
     and unchecked arithmetic on them wraps on a 32 bit target, which
     is where this guard earns its keep. */
  const auto full = MakeComposite();
  const auto whole = full.Get();
  for (std::size_t cut : {std::size_t{8}, whole.size() / 2,
                          whole.size() - 4}) {
    try {
      OPERA::ParseIndex(whole.first(cut));
      ok1(false);
    } catch (...) {
      ok1(true);
    }
  }

  /* --- level selection ------------------------------------------- */

  /* the coarsest level that still resolves what was asked for, to
     within the oversampling tolerance */
  ok1(OPERA::SelectLevel(index, 3000) == 1);
  ok1(OPERA::SelectLevel(index, 1500) == 1);

  /* the tolerance has to bite exactly at its own boundary, or the
     rule would be one the measurements do not describe: level 1 is
     2000 m, so it may serve a request down to 1000 m and no finer */
  ok1(OPERA::SelectLevel(index, 2000 / OPERA::LEVEL_TOLERANCE) == 1);
  ok1(OPERA::SelectLevel(index, 2000 / OPERA::LEVEL_TOLERANCE - 1) == 0);

  /* asking for more detail than the file holds must not fall off the
     bottom; the finest level is the best that can be done */
  ok1(OPERA::SelectLevel(index, 10) == 0);

  /* --- which tiles an area needs --------------------------------- */

  /* a small area around the projection centre falls in the last of
     the four tiles, the one the grid was laid out to put it in */
  const GeoBounds centre_area{GeoPoint{Angle::Degrees(10),
                                       Angle::Degrees(55)}};
  const auto needed = OPERA::CoveringSourceTiles(index.levels[0],
                                                 centre_area);
  ok1(needed.size() == 1);
  ok1(needed.front() == 3);

  /* the overview covers the same ground in one tile */
  const auto needed_coarse = OPERA::CoveringSourceTiles(index.levels[1],
                                                        centre_area);
  ok1(needed_coarse.size() == 1);
  ok1(needed_coarse.front() == 0);

  /* an area the composite does not reach asks for nothing at all,
     rather than for a tile that is not there */
  const GeoBounds far_away{GeoPoint{Angle::Degrees(-140),
                                    Angle::Degrees(-40)}};
  ok1(OPERA::CoveringSourceTiles(index.levels[0], far_away).empty());
  ok1(OPERA::CoveringSourceTiles(index.levels[0],
                                 GeoBounds::Invalid()).empty());

  return exit_status();
}
