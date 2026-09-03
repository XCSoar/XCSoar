// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Radar.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "Math/Constants.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace {

constexpr const char *BUCKET_URL =
  "https://s3.waw3-1.cloudferro.com/openradar-24h";

/* the Lambert azimuthal equal-area grid the composite is published
   on, read from the GeoTIFF's own keys */
constexpr double EARTH_RADIUS = 6378140;
constexpr double CENTRE_LATITUDE = 55;
constexpr double CENTRE_LONGITUDE = 10;
constexpr double FALSE_EASTING = 1950000;
constexpr double FALSE_NORTHING = -2100000;

/**
 * The lower dBZ bound of each colour class, obtained by matching the
 * quantiles of the composite against a DWD European radar image.
 *
 * Classes 1 and 9 were absent from that image, so the match left them
 * with an empty range; their bounds are interpolated evenly in dBZ
 * (that is, geometrically in rain rate) between the neighbours it did
 * determine.  The array must stay strictly increasing, or
 * ClassifyReflectivity() would step over a class and one colour would
 * never be drawn.
 */
constexpr std::array<double, OPERA::N_CLASSES> CLASS_MINIMUM{
  18.77, 21.16, 23.56, 25.95, 30.43, 34.62, 38.68, 42.82,
  47.81, 50.77, 53.73, 56.69, 60.70, 62.96, 66.14,
};

static_assert(std::ranges::adjacent_find(CLASS_MINIMUM,
                                         std::greater_equal<>{}) ==
              CLASS_MINIMUM.end(),
              "the class bounds must be strictly increasing");

constexpr std::array<uint32_t, OPERA::N_CLASSES> CLASS_COLOUR{
  0x33ffff, 0x1acc9a, 0x019934, 0x4db31b, 0x99cc01,
  0xcce601, 0xffff01, 0xffc401, 0xff8901, 0xff4501,
  0xfe0000, 0xe5004c, 0xcc0098, 0x6600cb, 0x0000fe,
};

/* ------------------------------------------------------------------
 * a very small TIFF directory reader
 *
 * Only what the composite is: a tiled, Deflate compressed, two band
 * float image with a pyramid of overviews.  It reads the head of the
 * file alone, so every offset has to be checked against how much of
 * the file we actually hold.
 * ------------------------------------------------------------------ */

constexpr unsigned TAG_WIDTH = 256, TAG_HEIGHT = 257;
constexpr unsigned TAG_COMPRESSION = 259, TAG_SAMPLES = 277;
constexpr unsigned TAG_TILE_WIDTH = 322, TAG_TILE_HEIGHT = 323;
constexpr unsigned TAG_TILE_OFFSETS = 324, TAG_TILE_LENGTHS = 325;
constexpr unsigned TAG_PIXEL_SCALE = 33550, TAG_TIEPOINT = 33922;
constexpr unsigned COMPRESSION_DEFLATE = 8;

/** how many image file directories to walk before giving up */
constexpr unsigned MAX_DIRECTORIES = 32;

/**
 * The most tiles a level may claim.  The composite's finest level has
 * 72; this is room for one a hundred times its size, and a bound on
 * what a corrupt count can make us reserve.
 */
constexpr uint32_t MAX_TILES = 65536;

class HeaderReader {
  std::span<const std::byte> header;
  bool big_endian = false;

public:
  explicit HeaderReader(std::span<const std::byte> _header)
    :header(_header)
  {
    if (header.size() < 8)
      throw std::runtime_error("Truncated radar image");

    const auto *p = (const uint8_t *)header.data();
    if (p[0] == 'M' && p[1] == 'M')
      big_endian = true;
    else if (p[0] != 'I' || p[1] != 'I')
      throw std::runtime_error("Not a TIFF radar image");

    /* 42 is a classic TIFF; a BigTIFF would be 43 and would need
       eight byte offsets, which this reader does not do */
    if (U16(2) != 42)
      throw std::runtime_error("Unsupported radar image format");
  }

  bool IsForeignByteOrder() const noexcept {
    return big_endian != (std::endian::native == std::endian::big);
  }

  /**
   * Is there room for @p length bytes at @p offset?
   *
   * Written as a subtraction rather than as `offset + length > size()`
   * because the offsets come out of the file: on a 32 bit target a
   * corrupt one near the top of the range would make that sum wrap
   * and the check pass.
   */
  bool Holds(std::size_t offset, std::size_t length) const noexcept {
    return length <= header.size() && offset <= header.size() - length;
  }

  uint16_t U16(std::size_t o) const {
    if (!Holds(o, 2))
      throw std::runtime_error("Truncated radar image");

    const auto *p = (const uint8_t *)header.data() + o;
    return big_endian ? uint16_t(p[0] << 8 | p[1]) : uint16_t(p[1] << 8 | p[0]);
  }

  uint32_t U32(std::size_t o) const {
    if (!Holds(o, 4))
      throw std::runtime_error("Truncated radar image");

    const auto *p = (const uint8_t *)header.data() + o;
    return big_endian
      ? (uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 |
         uint32_t(p[2]) << 8 | p[3])
      : (uint32_t(p[3]) << 24 | uint32_t(p[2]) << 16 |
         uint32_t(p[1]) << 8 | p[0]);
  }

  double F64(std::size_t o) const {
    /* U32() already applies the file byte order within each half, so
       all that is left is which half comes first */
    const uint64_t first = U32(o), second = U32(o + 4);
    return std::bit_cast<double>(big_endian
                                 ? first << 32 | second
                                 : second << 32 | first);
  }

  std::size_t FirstDirectory() const { return U32(4); }

  /**
   * @return the offset of the next directory, zero at the end
   */
  std::size_t ReadDirectory(std::size_t offset,
                            OPERA::CompositeLevel &level) const;
};

std::size_t
HeaderReader::ReadDirectory(std::size_t offset,
                            OPERA::CompositeLevel &level) const
{
  const unsigned n = U16(offset);

  /* validate the whole directory up front, so that the entry reads
     below cannot form an offset that wraps before it is checked */
  if (!Holds(offset, 2 + std::size_t(n) * 12 + 4))
    throw std::runtime_error("Truncated radar image");

  unsigned compression = 0;

  for (unsigned i = 0; i < n; ++i) {
    const std::size_t e = offset + 2 + std::size_t(i) * 12;
    const unsigned tag = U16(e), type = U16(e + 2);
    const uint32_t count = U32(e + 4);

    /* the value is inline when it fits in the four bytes of the entry */
    const std::size_t size = type == 3 ? 2 : (type == 12 ? 8 : 4);
    const std::size_t at = size * std::size_t(count) <= 4
      ? e + 8
      : U32(e + 8);

    const auto scalar = [this, type, at]() -> uint32_t {
      return type == 3 ? U16(at) : U32(at);
    };

    switch (tag) {
    case TAG_WIDTH: level.width = scalar(); break;
    case TAG_HEIGHT: level.height = scalar(); break;
    case TAG_COMPRESSION: compression = scalar(); break;
    case TAG_SAMPLES: level.samples = scalar(); break;
    case TAG_TILE_WIDTH: level.tile_width = scalar(); break;
    case TAG_TILE_HEIGHT: level.tile_height = scalar(); break;

    case TAG_TILE_OFFSETS:
    case TAG_TILE_LENGTHS: {
      /* the count comes out of the file; a corrupt one must not turn
         into a four gigabyte reservation */
      if (count > MAX_TILES)
        throw std::runtime_error("Unusable radar image");

      const std::size_t stride = type == 3 ? 2 : 4;
      if (!Holds(at, std::size_t(count) * stride))
        throw std::runtime_error("Truncated radar image");

      auto &out = tag == TAG_TILE_OFFSETS
        ? level.tile_offset
        : level.tile_length;
      out.clear();
      out.reserve(count);
      for (uint32_t j = 0; j < count; ++j)
        out.push_back(stride == 2
                      ? U16(at + std::size_t(j) * 2)
                      : U32(at + std::size_t(j) * 4));
      break;
    }

    case TAG_PIXEL_SCALE:
      if (count >= 2)
        level.scale = F64(at);
      break;

    case TAG_TIEPOINT:
      /* the first three doubles are the raster point, the next three
         the projected point it maps to */
      if (count >= 6) {
        level.origin_x = F64(at + 3 * 8);
        level.origin_y = F64(at + 4 * 8);
      }
      break;
    }
  }

  if (level.width > 0) {
    if (compression != COMPRESSION_DEFLATE)
      throw std::runtime_error("Unsupported radar image compression");

    if (level.samples == 0)
      /* the tag is mandatory, but a missing one would make us read
         zero bytes per pixel */
      level.samples = 1;
  }

  return U32(offset + 2 + std::size_t(n) * 12);
}

/** the extent an area occupies in the composite's projection */
struct ProjectedExtent {
  double min_x, max_x, min_y, max_y;
};

[[gnu::pure]]
ProjectedExtent
GetProjectedExtent(const GeoBounds &bounds) noexcept
{
  const double north = bounds.GetNorth().Degrees();
  const double south = bounds.GetSouth().Degrees();
  const double west = bounds.GetWest().Degrees();
  const double east = bounds.GetEast().Degrees();

  /* the projection is curved, so probe a coarse grid rather than just
     the corners to find the extent the area really needs */
  static constexpr unsigned PROBE = 4;
  ProjectedExtent extent{1e30, -1e30, 1e30, -1e30};

  for (unsigned j = 0; j <= PROBE; ++j)
    for (unsigned i = 0; i <= PROBE; ++i) {
      const auto p = OPERA::Project(GeoPoint{
        Angle::Degrees(west + (east - west) * i / PROBE),
        Angle::Degrees(south + (north - south) * j / PROBE),
      });
      extent.min_x = std::min(extent.min_x, p.x);
      extent.max_x = std::max(extent.max_x, p.x);
      extent.min_y = std::min(extent.min_y, p.y);
      extent.max_y = std::max(extent.max_y, p.y);
    }

  return extent;
}

} // anonymous namespace

OPERA::RasterProjector::RasterProjector(double west, double east,
                                        unsigned width)
{
  delta.resize(width);

  const double lon0 = CENTRE_LONGITUDE * M_PI / 180;
  for (unsigned x = 0; x < width; ++x) {
    const double longitude = west + (east - west) * (x + 0.5) / width;
    const double d = longitude * M_PI / 180 - lon0;
    delta[x] = {std::sin(d), std::cos(d)};
  }
}

void
OPERA::RasterProjector::ProjectRow(double latitude,
                                   DoublePoint2D *out) const noexcept
{
  const double lat = latitude * M_PI / 180;
  const double lat0 = CENTRE_LATITUDE * M_PI / 180;
  const double sin_lat = std::sin(lat), cos_lat = std::cos(lat);
  const double sin_lat0 = std::sin(lat0), cos_lat0 = std::cos(lat0);

  const double a = sin_lat0 * sin_lat;
  const double b = cos_lat0 * cos_lat;
  const double c = cos_lat0 * sin_lat;
  const double d = sin_lat0 * cos_lat;

  for (std::size_t x = 0; x < delta.size(); ++x) {
    const double sin_delta = delta[x].x, cos_delta = delta[x].y;

    const double denominator = 1 + a + b * cos_delta;
    if (denominator <= 0) {
      /* the antipode of the projection centre has no image */
      out[x] = {0, 0};
      continue;
    }

    const double k = EARTH_RADIUS * std::sqrt(2 / denominator);
    out[x] = {
      FALSE_EASTING + k * cos_lat * sin_delta,
      FALSE_NORTHING + k * (c - d * cos_delta),
    };
  }
}

double
OPERA::TileMetresPerPixel(const GeoBounds &bounds, unsigned pixels) noexcept
{
  if (!bounds.IsValid() || pixels == 0)
    return 0;

  const auto extent = GetProjectedExtent(bounds);

  /* the wider of the two axes, so that neither ends up undersampled
     by a level chosen for the other */
  return std::max(extent.max_x - extent.min_x,
                  extent.max_y - extent.min_y) / pixels;
}

BrokenDateTime
OPERA::CompositeTime(const BrokenDateTime &utc)
{
  if (!utc.IsPlausible())
    /* without a clock we cannot tell which composite is the current
       one, and guessing would show yesterday's weather */
    return BrokenDateTime::Invalid();

  auto t = utc - std::chrono::minutes{LATENCY_MINUTES};
  t.minute = (t.minute / CADENCE_MINUTES) * CADENCE_MINUTES;
  t.second = 0;
  return t;
}

std::string
OPERA::MakeCompositeURL(const BrokenDateTime &utc)
{
  const auto t = CompositeTime(utc);
  if (!t.IsPlausible())
    return {};

  return fmt::format("{}/{:04}/{:02}/{:02}/OPERA/COMP/"
                     "OPERA@{:04}{:02}{:02}T{:02}{:02}@0@DBZH.tiff",
                     BUCKET_URL, t.year, t.month, t.day,
                     t.year, t.month, t.day, t.hour, t.minute);
}

DoublePoint2D
OPERA::Project(const GeoPoint &p) noexcept
{
  const double lat = p.latitude.Radians();
  const double lon = p.longitude.Radians();
  const double lat0 = CENTRE_LATITUDE * M_PI / 180;
  const double delta = lon - CENTRE_LONGITUDE * M_PI / 180;

  const double denominator = 1 + std::sin(lat0) * std::sin(lat) +
    std::cos(lat0) * std::cos(lat) * std::cos(delta);
  if (denominator <= 0)
    /* the antipode of the projection centre has no image */
    return {0, 0};

  const double k = EARTH_RADIUS * std::sqrt(2 / denominator);

  return {
    FALSE_EASTING + k * std::cos(lat) * std::sin(delta),
    FALSE_NORTHING + k * (std::cos(lat0) * std::sin(lat) -
                          std::sin(lat0) * std::cos(lat) * std::cos(delta)),
  };
}

bool
OPERA::IsNotANumber(double value) noexcept
{
  uint64_t bits;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(bits));

  return ((bits >> 52) & 0x7ff) == 0x7ff && (bits & 0xfffffffffffffULL) != 0;
}

int
OPERA::ClassifyReflectivity(double dbz) noexcept
{
  if (IsNotANumber(dbz) || dbz < CLASS_MINIMUM.front())
    return -1;

  unsigned i = 0;
  while (i + 1 < N_CLASSES && dbz >= CLASS_MINIMUM[i + 1])
    ++i;

  return int(i);
}

uint32_t
OPERA::GetClassColour(unsigned i) noexcept
{
  return CLASS_COLOUR[i < N_CLASSES ? i : N_CLASSES - 1];
}

GeoBitmap::TileData
OPERA::GetAircraftTile(const GeoPoint &location, uint16_t zoom) noexcept
{
  if (!location.IsValid() || zoom < MIN_TILE_ZOOM || zoom > MAX_TILE_ZOOM)
    return {};

  return GeoBitmap::GetTile(GeoBounds{location}, zoom);
}

std::vector<GeoBitmap::TileData>
OPERA::CollectTiles(const GeoBitmap::TileData &base) noexcept
{
  std::vector<GeoBitmap::TileData> result;
  if (!base.IsValid())
    return result;

  const int tiles_per_axis = 1 << base.zoom;

  /* longitude wraps, so a flight over the date line keeps a complete
     block instead of losing half of it */
  const auto normalise_x = [tiles_per_axis](int value) {
    int wrapped = value % tiles_per_axis;
    if (wrapped < 0)
      wrapped += tiles_per_axis;

    return uint32_t(wrapped);
  };

  struct Candidate {
    GeoBitmap::TileData tile;
    unsigned priority;
  };

  std::vector<Candidate> candidates;
  candidates.reserve(TILE_COUNT);

  const int range = int(TILE_RANGE);
  for (int dx = -range; dx <= range; ++dx) {
    for (int dy = -range; dy <= range; ++dy) {
      const int y = int(base.y) + dy;
      if (y < 0 || y >= tiles_per_axis)
        /* latitude does not wrap: off the top or bottom of the world
           there is no tile to ask for */
        continue;

      candidates.push_back({
        {base.zoom, normalise_x(int(base.x) + dx), uint32_t(y)},
        unsigned(dx * dx + dy * dy),
      });
    }
  }

  /* squared distance from the aircraft's own tile, so the block is
     walked as rings growing outwards from underneath the glider */
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const auto &a, const auto &b) {
                     return a.priority < b.priority;
                   });

  result.reserve(candidates.size());
  for (const auto &candidate : candidates)
    result.push_back(candidate.tile);

  return result;
}

OPERA::CompositeIndex
OPERA::ParseIndex(std::span<const std::byte> header)
{
  const HeaderReader reader{header};

  CompositeIndex index;
  index.foreign_byte_order = reader.IsForeignByteOrder();

  /* count every directory, not just the usable ones: a damaged file
     can chain empty directories, or point one at itself */
  for (std::size_t offset = reader.FirstDirectory(), n = 0;
       offset != 0 && n < MAX_DIRECTORIES; ++n) {
    CompositeLevel level;
    offset = reader.ReadDirectory(offset, level);
    if (level.width > 0)
      index.levels.push_back(std::move(level));
  }

  if (index.levels.empty())
    throw std::runtime_error("Empty radar image");

  /* only the full resolution image carries the projection keys; the
     overviews cover the same area with fewer pixels, so their scale
     follows from the size ratio */
  const auto &full = index.levels.front();
  if (full.scale <= 0)
    throw std::runtime_error("Radar image without a georeference");

  for (auto &level : index.levels) {
    if (level.scale > 0 || level.width == 0)
      continue;

    level.scale = full.scale * double(full.width) / level.width;
    level.origin_x = full.origin_x;
    level.origin_y = full.origin_y;
  }

  return index;
}

unsigned
OPERA::SelectLevel(const CompositeIndex &index,
                   double metres_per_pixel) noexcept
{
  unsigned best = 0;

  const double limit = metres_per_pixel * LEVEL_TOLERANCE;

  for (unsigned i = 0; i < index.levels.size(); ++i)
    if (index.levels[i].IsUsable() && index.levels[i].scale <= limit)
      /* the levels are ordered fine to coarse, so the last one that
         is still fine enough is the cheapest usable one */
      best = i;

  return best;
}

std::vector<unsigned>
OPERA::CoveringSourceTiles(const CompositeLevel &level,
                           const GeoBounds &bounds) noexcept
{
  std::vector<unsigned> result;
  if (!level.IsUsable() || !bounds.IsValid())
    return result;

  const auto extent = GetProjectedExtent(bounds);

  const double c0 = (extent.min_x - level.origin_x) / level.scale;
  const double c1 = (extent.max_x - level.origin_x) / level.scale;
  const double r0 = (level.origin_y - extent.max_y) / level.scale;
  const double r1 = (level.origin_y - extent.min_y) / level.scale;

  if (c1 < 0 || r1 < 0 ||
      c0 >= double(level.width) || r0 >= double(level.height))
    /* the area is somewhere the composite does not cover */
    return result;

  const unsigned across = level.TilesAcross(), down = level.TilesDown();
  const unsigned tx0 = unsigned(std::max(0.0, c0)) / level.tile_width;
  const unsigned ty0 = unsigned(std::max(0.0, r0)) / level.tile_height;
  const unsigned tx1 =
    unsigned(std::min(double(level.width - 1), c1)) / level.tile_width;
  const unsigned ty1 =
    unsigned(std::min(double(level.height - 1), r1)) / level.tile_height;

  for (unsigned ty = ty0; ty <= ty1 && ty < down; ++ty)
    for (unsigned tx = tx0; tx <= tx1 && tx < across; ++tx) {
      const unsigned i = ty * across + tx;
      if (i < level.tile_offset.size() && level.tile_length[i] > 0)
        result.push_back(i);
    }

  return result;
}
