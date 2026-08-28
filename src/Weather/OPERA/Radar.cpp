// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Radar.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "net/http/CoDownloadToFile.hpp"
#include "co/Task.hxx"
#include "io/FileMapping.hpp"
#include "io/FileOutputStream.hxx"
#include "time/BrokenDateTime.hpp"
#include "util/AllocatedArray.hxx"
#include "util/ByteOrder.hxx"
#include "LocalPath.hpp"

#include <zlib.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

/**
 * The subset of TIFF we need: the composite is a tiled, Deflate
 * compressed, two band float image with a pyramid of overviews.
 */
/**
 * Stands in for "no measurement here".  A plain low value rather than
 * a NaN, because we are built with -ffast-math and must not rely on
 * NaN comparisons behaving.
 */
constexpr float NO_ECHO = -1e30f;

struct TiffLevel {
  unsigned width = 0, height = 0;
  unsigned tile_width = 0, tile_height = 0;
  unsigned samples = 0;
  std::vector<uint32_t> tile_offset, tile_length;
  double scale = 0;
  double origin_x = 0, origin_y = 0;

  unsigned TilesAcross() const noexcept {
    return (width + tile_width - 1) / tile_width;
  }
};

class TiffReader {
  std::span<const std::byte> file;
  bool big_endian = false;

public:
  explicit TiffReader(std::span<const std::byte> _file):file(_file) {
    if (file.size() < 8)
      throw std::runtime_error("Truncated radar image");

    const auto *p = (const uint8_t *)file.data();
    if (p[0] == 'M' && p[1] == 'M')
      big_endian = true;
    else if (p[0] != 'I' || p[1] != 'I')
      throw std::runtime_error("Not a TIFF radar image");
  }

  uint16_t U16(std::size_t o) const {
    if (o + 2 > file.size())
      throw std::runtime_error("Truncated radar image");
    const auto *p = (const uint8_t *)file.data() + o;
    return big_endian ? uint16_t(p[0] << 8 | p[1]) : uint16_t(p[1] << 8 | p[0]);
  }

  uint32_t U32(std::size_t o) const {
    if (o + 4 > file.size())
      throw std::runtime_error("Truncated radar image");
    const auto *p = (const uint8_t *)file.data() + o;
    return big_endian
      ? (uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3])
      : (uint32_t(p[3]) << 24 | uint32_t(p[2]) << 16 | uint32_t(p[1]) << 8 | p[0]);
  }

  double F64(std::size_t o) const {
    /* U32() already applies the file byte order within each half, so
       all that is left is which half comes first */
    const uint64_t first = U32(o), second = U32(o + 4);
    return std::bit_cast<double>(big_endian
                                 ? first << 32 | second
                                 : second << 32 | first);
  }

  bool IsForeignByteOrder() const noexcept {
    return big_endian != (std::endian::native == std::endian::big);
  }

  std::span<const std::byte> Slice(std::size_t o, std::size_t n) const {
    if (o + n > file.size())
      throw std::runtime_error("Truncated radar image");
    return file.subspan(o, n);
  }

  std::size_t FirstDirectory() const { return U32(4); }

  /**
   * Read one image file directory.
   *
   * @return the offset of the next one, zero at the end
   */
  std::size_t ReadDirectory(std::size_t offset, TiffLevel &level) const;
};

constexpr unsigned TAG_WIDTH = 256, TAG_HEIGHT = 257;
constexpr unsigned TAG_COMPRESSION = 259, TAG_SAMPLES = 277;
constexpr unsigned TAG_TILE_WIDTH = 322, TAG_TILE_HEIGHT = 323;
constexpr unsigned TAG_TILE_OFFSETS = 324, TAG_TILE_LENGTHS = 325;
constexpr unsigned TAG_PIXEL_SCALE = 33550, TAG_TIEPOINT = 33922;
constexpr unsigned COMPRESSION_DEFLATE = 8;

std::size_t
TiffReader::ReadDirectory(std::size_t offset, TiffLevel &level) const
{
  const unsigned n = U16(offset);
  unsigned compression = 0;

  for (unsigned i = 0; i < n; ++i) {
    const std::size_t e = offset + 2 + i * 12;
    const unsigned tag = U16(e), type = U16(e + 2);
    const uint32_t count = U32(e + 4);

    /* the value is inline when it fits in four bytes */
    const std::size_t size = type == 3 ? 2 : (type == 12 ? 8 : 4);
    const std::size_t at = std::size_t(size) * count <= 4 ? e + 8 : U32(e + 8);

    const auto scalar = [&]() -> uint32_t {
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
      auto &out = tag == TAG_TILE_OFFSETS ? level.tile_offset : level.tile_length;
      out.clear();
      out.reserve(count);
      for (uint32_t j = 0; j < count; ++j)
        out.push_back(type == 3 ? U16(at + j * 2) : U32(at + j * 4));
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

/**
 * Uncompress one tile and return its first band.
 */
void
InflateTile(std::span<const std::byte> tile, const TiffLevel &level,
            bool foreign_byte_order, float *out)
{
  const std::size_t pixels = std::size_t(level.tile_width) * level.tile_height;
  AllocatedArray<float> raw{pixels * level.samples};

  uLongf size = uLongf(raw.size() * sizeof(float));
  if (::uncompress((Bytef *)raw.data(), &size,
                   (const Bytef *)tile.data(), uLong(tile.size())) != Z_OK ||
      size < pixels * level.samples * sizeof(float))
    throw std::runtime_error("Damaged radar image");

  for (std::size_t i = 0; i < pixels; ++i) {
    const float value = raw[i * level.samples];
    out[i] = foreign_byte_order
      ? std::bit_cast<float>(ByteSwap32(std::bit_cast<uint32_t>(value)))
      : value;
  }
}

} // anonymous namespace

namespace {

/**
 * Pick the coarsest overview that still resolves the requested
 * detail.  Asking for a finer one only costs bandwidth and memory.
 */
const TiffLevel &
SelectLevel(const std::vector<TiffLevel> &levels,
            double metres_per_pixel) noexcept
{
  const TiffLevel *best = &levels.front();

  for (const auto &level : levels)
    if (level.width > 0 && level.scale > 0 &&
        level.scale <= metres_per_pixel)
      /* the levels are ordered fine to coarse, so the last one that
         is still fine enough is the cheapest usable one */
      best = &level;

  return *best;
}

/**
 * The part of one level we need, decoded into a contiguous grid.
 */
class Patch {
  unsigned x0 = 0, y0 = 0, width = 0, height = 0;
  AllocatedArray<float> data;

public:
  Patch(const TiffReader &tiff, const TiffLevel &level,
        unsigned col0, unsigned row0, unsigned col1, unsigned row1);

  /**
   * @return the reflectivity, or #NO_ECHO where there is none
   */
  float Get(unsigned col, unsigned row) const noexcept {
    if (col < x0 || row < y0)
      return NO_ECHO;

    const unsigned x = col - x0, y = row - y0;
    if (x >= width || y >= height)
      return NO_ECHO;

    return data[std::size_t(y) * width + x];
  }
};

Patch::Patch(const TiffReader &tiff, const TiffLevel &level,
             unsigned col0, unsigned row0, unsigned col1, unsigned row1)
{
  const unsigned across = level.TilesAcross();
  const unsigned tx0 = col0 / level.tile_width, tx1 = col1 / level.tile_width;
  const unsigned ty0 = row0 / level.tile_height, ty1 = row1 / level.tile_height;

  x0 = tx0 * level.tile_width;
  y0 = ty0 * level.tile_height;
  width = (tx1 - tx0 + 1) * level.tile_width;
  height = (ty1 - ty0 + 1) * level.tile_height;

  data.ResizeDiscard(std::size_t(width) * height);
  std::fill_n(data.data(), data.size(), NO_ECHO);

  AllocatedArray<float> tile{std::size_t(level.tile_width) * level.tile_height};

  for (unsigned ty = ty0; ty <= ty1; ++ty) {
    for (unsigned tx = tx0; tx <= tx1; ++tx) {
      const std::size_t i = std::size_t(ty) * across + tx;
      if (i >= level.tile_offset.size() || i >= level.tile_length.size() ||
          level.tile_length[i] == 0)
        continue;

      InflateTile(tiff.Slice(level.tile_offset[i], level.tile_length[i]),
                  level, tiff.IsForeignByteOrder(), tile.data());

      for (unsigned y = 0; y < level.tile_height; ++y) {
        const unsigned dy = (ty - ty0) * level.tile_height + y;
        if (dy >= height)
          break;

        const float *src = tile.data() + std::size_t(y) * level.tile_width;
        float *dest = data.data() + std::size_t(dy) * width +
          (tx - tx0) * level.tile_width;

        for (unsigned x = 0; x < level.tile_width; ++x)
          /* the composite marks "looked and saw nothing" with a NaN
             and "no radar here" with a large negative number */
          dest[x] = OPERA::IsNotANumber(src[x]) ? NO_ECHO : src[x];
      }
    }
  }
}

/**
 * Write an RGBA image as a PNG.  Throws on error.
 */
void
WritePNG(Path path, unsigned width, unsigned height, const uint8_t *rgba)
{
  /* one filter byte per row, filter type 0 ("none") */
  AllocatedArray<uint8_t> raw{std::size_t(height) * (1 + std::size_t(width) * 4)};
  for (unsigned y = 0; y < height; ++y) {
    uint8_t *dest = raw.data() + std::size_t(y) * (1 + std::size_t(width) * 4);
    *dest++ = 0;
    std::copy_n(rgba + std::size_t(y) * width * 4, std::size_t(width) * 4, dest);
  }

  uLongf compressed_size = ::compressBound(uLong(raw.size()));
  AllocatedArray<uint8_t> compressed{compressed_size};
  if (::compress2(compressed.data(), &compressed_size,
                  raw.data(), uLong(raw.size()), 6) != Z_OK)
    throw std::runtime_error("Failed to compress the radar image");

  FileOutputStream file{path, FileOutputStream::Mode::CREATE};

  const auto WriteChunk = [&file](const char *type,
                                  const uint8_t *payload, std::size_t size){
    uint8_t header[8];
    header[0] = uint8_t(size >> 24); header[1] = uint8_t(size >> 16);
    header[2] = uint8_t(size >> 8); header[3] = uint8_t(size);
    std::memcpy(header + 4, type, 4);
    file.Write(std::as_bytes(std::span{header}));
    if (size > 0)
      file.Write(std::as_bytes(std::span{payload, size}));

    uLong crc = ::crc32(0, header + 4, 4);
    if (size > 0)
      crc = ::crc32(crc, payload, uInt(size));

    const uint8_t tail[4] = {
      uint8_t(crc >> 24), uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc),
    };
    file.Write(std::as_bytes(std::span{tail}));
  };

  static constexpr uint8_t signature[] = {
    0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a,
  };
  file.Write(std::as_bytes(std::span{signature}));

  const uint8_t ihdr[13] = {
    uint8_t(width >> 24), uint8_t(width >> 16), uint8_t(width >> 8), uint8_t(width),
    uint8_t(height >> 24), uint8_t(height >> 16), uint8_t(height >> 8), uint8_t(height),
    8, 6, 0, 0, 0,
  };
  WriteChunk("IHDR", ihdr, sizeof(ihdr));
  WriteChunk("IDAT", compressed.data(), compressed_size);
  WriteChunk("IEND", nullptr, 0);

  file.Commit();
}

} // anonymous namespace

namespace {

/**
 * Reproject the composite into a north-up latitude/longitude image
 * and colour it.
 */
AllocatedArray<uint8_t>
Render(const TiffReader &tiff, const std::vector<TiffLevel> &levels,
       const GeoBounds &bounds, unsigned width, unsigned height)
{
  const double north = bounds.GetNorth().Degrees();
  const double south = bounds.GetSouth().Degrees();
  const double west = bounds.GetWest().Degrees();
  const double east = bounds.GetEast().Degrees();

  /* the projection is curved, so probe a coarse grid rather than
     just the corners to find the extent we need */
  static constexpr unsigned PROBE = 8;
  double min_x = 1e30, max_x = -1e30, min_y = 1e30, max_y = -1e30;
  for (unsigned j = 0; j <= PROBE; ++j) {
    for (unsigned i = 0; i <= PROBE; ++i) {
      const auto p = OPERA::Project(GeoPoint{
        Angle::Degrees(west + (east - west) * i / PROBE),
        Angle::Degrees(south + (north - south) * j / PROBE),
      });
      min_x = std::min(min_x, p.x); max_x = std::max(max_x, p.x);
      min_y = std::min(min_y, p.y); max_y = std::max(max_y, p.y);
    }
  }

  const auto &level = SelectLevel(levels, (max_x - min_x) / width);
  if (level.width == 0 || level.scale <= 0 ||
      level.tile_width == 0 || level.tile_height == 0)
    throw std::runtime_error("Unusable radar image");

  const auto ToColumn = [&level](double x){
    return (x - level.origin_x) / level.scale;
  };
  const auto ToRow = [&level](double y){
    return (level.origin_y - y) / level.scale;
  };

  const double c0 = ToColumn(min_x), c1 = ToColumn(max_x);
  const double r0 = ToRow(max_y), r1 = ToRow(min_y);
  if (c1 < 0 || r1 < 0 || c0 >= level.width || r0 >= level.height)
    /* the map is looking somewhere the composite does not cover */
    throw std::runtime_error("Outside the radar composite");

  const Patch patch{tiff, level,
                    unsigned(std::max(0.0, c0)),
                    unsigned(std::max(0.0, r0)),
                    unsigned(std::min(double(level.width - 1), c1)),
                    unsigned(std::min(double(level.height - 1), r1))};

  AllocatedArray<uint8_t> image{std::size_t(width) * height * 4};
  std::fill_n(image.data(), image.size(), uint8_t(0));

  for (unsigned y = 0; y < height; ++y) {
    const double latitude = north - (north - south) * (y + 0.5) / height;

    for (unsigned x = 0; x < width; ++x) {
      const double longitude = west + (east - west) * (x + 0.5) / width;

      const auto p = OPERA::Project(GeoPoint{Angle::Degrees(longitude),
                                             Angle::Degrees(latitude)});
      const double column = ToColumn(p.x), row = ToRow(p.y);
      if (column < 0 || row < 0 ||
          column >= level.width || row >= level.height)
        continue;

      /* take the strongest of the source pixels this one covers; the
         composite is a maximum product, and picking just one would
         drop isolated echoes when zoomed out */
      float dbz = NO_ECHO;
      for (unsigned dy = 0; dy < 2; ++dy)
        for (unsigned dx = 0; dx < 2; ++dx)
          dbz = std::max(dbz, patch.Get(unsigned(column) + dx,
                                        unsigned(row) + dy));

      const int cls = OPERA::ClassifyReflectivity(dbz);
      if (cls < 0)
        continue;

      const uint32_t colour = OPERA::GetClassColour(unsigned(cls));
      uint8_t *dest = image.data() + (std::size_t(y) * width + x) * 4;
      dest[0] = uint8_t(colour >> 16);
      dest[1] = uint8_t(colour >> 8);
      dest[2] = uint8_t(colour);
      dest[3] = 0xff;
    }
  }

  return image;
}

} // anonymous namespace

Co::Task<AllocatedPath>
OPERA::DownloadRadar(const GeoBounds &bounds, unsigned width, unsigned height,
                     CurlGlobal &curl, ProgressListener &progress)
{
  if (!bounds.IsValid())
    throw std::runtime_error("No map area to cover");

  width = std::clamp(width, 1u, MAX_IMAGE_SIZE);
  height = std::clamp(height, 1u, MAX_IMAGE_SIZE);

  const auto url = MakeCompositeURL(BrokenDateTime::NowUTC());
  if (url.empty())
    throw std::runtime_error("The clock is not set");

  const auto cache = MakeCacheDirectory("opera");
  const auto source = AllocatedPath::Build(cache, "composite.tiff");
  auto rendered = AllocatedPath::Build(cache, "composite.png");

  {
    const auto ignored_response = co_await
      Net::CoDownloadToFile(curl, url.c_str(), nullptr, nullptr,
                            source, nullptr, progress);
  }

  const FileMapping mapping{source};
  const TiffReader tiff{mapping};

  std::vector<TiffLevel> levels;
  /* count every directory, not just the usable ones: a damaged file
     can chain empty directories, or point one at itself */
  for (std::size_t offset = tiff.FirstDirectory(), n = 0;
       offset != 0 && n < 32; ++n) {
    TiffLevel level;
    offset = tiff.ReadDirectory(offset, level);
    if (level.width > 0)
      levels.push_back(std::move(level));
  }

  if (levels.empty())
    throw std::runtime_error("Empty radar image");

  /* only the full resolution image carries the projection keys; the
     overviews cover the same area with fewer pixels, so their scale
     follows from the size ratio */
  for (auto &level : levels) {
    if (level.scale > 0 || level.width == 0 || levels.front().width == 0)
      continue;

    level.scale = levels.front().scale *
      double(levels.front().width) / level.width;
    level.origin_x = levels.front().origin_x;
    level.origin_y = levels.front().origin_y;
  }

  if (levels.front().scale <= 0)
    throw std::runtime_error("Radar image without a georeference");

  const auto image = Render(tiff, levels, bounds, width, height);
  WritePNG(rendered, width, height, image.data());

  co_return std::move(rendered);
}
