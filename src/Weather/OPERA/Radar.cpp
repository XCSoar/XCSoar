// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Radar.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Point2D.hpp"
#include "net/http/CoGetRange.hpp"
#include "co/Task.hxx"
#include "io/FileOutputStream.hxx"
#include "util/AllocatedArray.hxx"
#include "util/ByteOrder.hxx"
#include "time/BrokenDateTime.hpp"
#include "LocalPath.hpp"
#include "Operation/ProgressListener.hpp"

#include <zlib.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {

/** The class of a pixel with no echo, or none we hold. */
constexpr int8_t NO_CLASS = -1;

/**
 * The largest tile we will inflate, counted in samples rather than in
 * pixels, as a guard against a directory that claims an absurd tile
 * size or band count.  The composite's own tiles are 512 by 512 of
 * two float bands, half a million samples.
 */
constexpr std::size_t MAX_TILE_PIXELS = 4u * 1024 * 1024;

/** the largest single image the Weather dialog will draw */
constexpr unsigned MAX_AREA_PIXELS = 1024;

/**
 * Uncompress one tile, take its first band and classify it.
 *
 * The classification happens here rather than at draw time so that
 * what is kept is one byte per pixel instead of four, and so that a
 * pixel is classified once however many times it is drawn.
 */
std::vector<int8_t>
InflateTile(std::span<const std::byte> compressed,
            const OPERA::CompositeLevel &level, bool foreign_byte_order)
{
  const std::size_t pixels =
    std::size_t(level.tile_width) * level.tile_height;

  /* the sample count comes out of the file just as the tile size
     does, so the buffer has to be bounded by the product rather than
     by the pixel count alone; written as a division so that the check
     itself cannot overflow */
  if (pixels == 0 || level.samples == 0 ||
      pixels > MAX_TILE_PIXELS / level.samples)
    throw std::runtime_error("Unusable radar image");

  AllocatedArray<float> raw{pixels * level.samples};

  uLongf size = uLongf(raw.size() * sizeof(float));
  if (::uncompress((Bytef *)raw.data(), &size,
                   (const Bytef *)compressed.data(),
                   uLong(compressed.size())) != Z_OK ||
      size < pixels * level.samples * sizeof(float))
    throw std::runtime_error("Damaged radar image");

  std::vector<int8_t> out;
  out.resize(pixels);

  for (std::size_t i = 0; i < pixels; ++i) {
    float value = raw[i * level.samples];
    if (foreign_byte_order)
      value = std::bit_cast<float>(ByteSwap32(std::bit_cast<uint32_t>(value)));

    /* the composite marks "looked and saw nothing" with a NaN and "no
       radar here" with a large negative number; both come out as the
       "nothing to draw" class */
    out[i] = OPERA::IsNotANumber(value)
      ? NO_CLASS
      : int8_t(OPERA::ClassifyReflectivity(value));
  }

  return out;
}

/**
 * Write an RGBA image as a PNG.  Throws on error.
 */
void
WritePNG(Path path, unsigned width, unsigned height, const uint8_t *rgba)
{
  /* one filter byte per row, filter type 0 ("none") */
  AllocatedArray<uint8_t> raw{
    std::size_t(height) * (1 + std::size_t(width) * 4)};
  for (unsigned y = 0; y < height; ++y) {
    uint8_t *dest = raw.data() + std::size_t(y) * (1 + std::size_t(width) * 4);
    *dest++ = 0;
    std::copy_n(rgba + std::size_t(y) * width * 4, std::size_t(width) * 4,
                dest);
  }

  uLongf compressed_size = ::compressBound(uLong(raw.size()));
  AllocatedArray<uint8_t> compressed{compressed_size};
  if (::compress2(compressed.data(), &compressed_size,
                  raw.data(), uLong(raw.size()), 6) != Z_OK)
    throw std::runtime_error("Failed to compress the radar image");

  FileOutputStream file{path, FileOutputStream::Mode::CREATE};

  const auto write_chunk = [&file](const char *type,
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

  static constexpr uint8_t SIGNATURE[] = {
    0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a,
  };
  file.Write(std::as_bytes(std::span{SIGNATURE}));

  const uint8_t ihdr[13] = {
    uint8_t(width >> 24), uint8_t(width >> 16),
    uint8_t(width >> 8), uint8_t(width),
    uint8_t(height >> 24), uint8_t(height >> 16),
    uint8_t(height >> 8), uint8_t(height),
    8, 6, 0, 0, 0,
  };
  write_chunk("IHDR", ihdr, sizeof(ihdr));
  write_chunk("IDAT", compressed.data(), compressed_size);
  write_chunk("IEND", nullptr, 0);

  file.Commit();
}

} // anonymous namespace

bool
OPERA::RadarComposite::HasTile(unsigned level, unsigned tile) const noexcept
{
  return std::any_of(tiles.begin(), tiles.end(),
                     [level, tile](const auto &i){
                       return i.level == level && i.index == tile;
                     });
}

Co::Task<void>
OPERA::RadarComposite::Open(std::string _url, CurlGlobal &curl)
{
  if (IsOpen() && url == _url)
    co_return;

  Reset();

  /* one range request buys the whole directory: what levels the file
     has, how it is tiled, and where every tile of it lies */
  const auto header = co_await Net::CoGetRange(curl, _url.c_str(),
                                               0, INDEX_BYTES);

  index = ParseIndex(std::as_bytes(std::span{header}));
  url = std::move(_url);
}

Co::Task<void>
OPERA::RadarComposite::FetchTile(unsigned level_index, unsigned tile,
                                 CurlGlobal &curl)
{
  if (!IsOpen() || HasTile(level_index, tile))
    co_return;

  if (level_index >= index.levels.size())
    throw std::runtime_error("Unusable radar image");

  const auto &level = index.levels[level_index];
  if (!level.IsUsable() || tile >= level.tile_offset.size())
    throw std::runtime_error("Unusable radar image");

  const auto length = level.tile_length[tile];
  if (length == 0)
    /* an empty tile is not an error: the file marks areas the radar
       network does not reach that way */
    co_return;

  const auto compressed = co_await Net::CoGetRange(curl, url.c_str(),
                                                   level.tile_offset[tile],
                                                   length);
  if (compressed.size() < length)
    throw std::runtime_error("Truncated radar image");

  auto data = InflateTile(std::as_bytes(std::span{compressed}), level,
                          index.foreign_byte_order);

  /* the awaits above can have run concurrently with another fetch of
     the same tile; keep only one copy */
  if (HasTile(level_index, tile))
    co_return;

  /* the oldest goes first, which is the one the aircraft flew away
     from: the block is filled from its centre outwards, so what was
     fetched longest ago is what is furthest behind */
  while (tiles.size() >= MAX_CACHED_TILES)
    tiles.erase(tiles.begin());

  tiles.push_back({level_index, tile, std::move(data)});
}

bool
OPERA::RadarComposite::Render(unsigned level_index, const GeoBounds &bounds,
                              unsigned width, unsigned height,
                              Path path) const
{
  if (!IsOpen() || level_index >= index.levels.size() ||
      !bounds.IsValid() || width == 0 || height == 0)
    throw std::runtime_error("Unusable radar image");

  const auto &level = index.levels[level_index];
  if (!level.IsUsable())
    throw std::runtime_error("Unusable radar image");

  /* the handful of source tiles this area touches, looked up once
     rather than searched for per pixel */
  struct Held {
    unsigned index;
    const std::vector<int8_t> *data;
  };

  /* only the tiles this area actually touches, so that the lookup in
     the pixel loop below scans four entries rather than the whole
     cache */
  std::vector<Held> held;
  for (const auto source : CoveringSourceTiles(level, bounds))
    for (const auto &tile : tiles)
      if (tile.level == level_index && tile.index == source)
        held.push_back({tile.index, &tile.data});

  if (held.empty())
    /* nothing of this area has arrived yet, or the composite does not
       reach it */
    return false;

  const unsigned across = level.TilesAcross();

  const auto sample = [&](double column, double row) -> int8_t {
    if (column < 0 || row < 0 ||
        column >= double(level.width) || row >= double(level.height))
      return NO_CLASS;

    const unsigned c = unsigned(column), r = unsigned(row);
    const unsigned i = (r / level.tile_height) * across + c / level.tile_width;

    for (const auto &tile : held)
      if (tile.index == i) {
        const std::size_t x = c % level.tile_width;
        const std::size_t y = r % level.tile_height;
        const std::size_t at = y * level.tile_width + x;
        return at < tile.data->size() ? (*tile.data)[at] : NO_CLASS;
      }

    return NO_CLASS;
  };

  const double north = bounds.GetNorth().Degrees();
  const double south = bounds.GetSouth().Degrees();

  const RasterProjector projector{bounds.GetWest().Degrees(),
                                  bounds.GetEast().Degrees(), width};
  AllocatedArray<DoublePoint2D> row_points{width};

  AllocatedArray<uint8_t> image{std::size_t(width) * height * 4};
  std::fill_n(image.data(), image.size(), uint8_t(0));

  bool any = false;

  for (unsigned y = 0; y < height; ++y) {
    const double latitude = north - (north - south) * (y + 0.5) / height;
    projector.ProjectRow(latitude, row_points.data());

    for (unsigned x = 0; x < width; ++x) {
      const auto &p = row_points[x];
      const double column = (p.x - level.origin_x) / level.scale;
      const double row = (level.origin_y - p.y) / level.scale;

      /* take the strongest of the source pixels this one covers; the
         composite is a maximum product, and picking just one would
         drop isolated echoes when zoomed out.  The classification is
         monotonic, so the strongest class is the class of the
         strongest echo. */
      int8_t cls = NO_CLASS;
      for (unsigned dy = 0; dy < 2; ++dy)
        for (unsigned dx = 0; dx < 2; ++dx)
          cls = std::max(cls, sample(column + dx, row + dy));

      if (cls < 0)
        continue;

      const uint32_t colour = GetClassColour(unsigned(cls));
      uint8_t *dest = image.data() + (std::size_t(y) * width + x) * 4;
      dest[0] = uint8_t(colour >> 16);
      dest[1] = uint8_t(colour >> 8);
      dest[2] = uint8_t(colour);
      dest[3] = 0xff;
      any = true;
    }
  }

  WritePNG(path, width, height, image.data());

  /* the caller uses this to leave a clear sky alone rather than spend
     a map overlay slot and a texture upload on a transparent tile */
  return any;
}

Co::Task<AllocatedPath>
OPERA::DownloadArea(const GeoBounds &bounds, unsigned width, unsigned height,
                    CurlGlobal &curl, ProgressListener &progress)
{
  if (!bounds.IsValid())
    throw std::runtime_error("No map area to cover");

  width = std::clamp(width, 1u, MAX_AREA_PIXELS);
  height = std::clamp(height, 1u, MAX_AREA_PIXELS);

  const auto url = MakeCompositeURL(BrokenDateTime::NowUTC());
  if (url.empty())
    throw std::runtime_error("The clock is not set");

  RadarComposite composite;
  co_await composite.Open(url, curl);

  const auto &index = composite.GetIndex();
  const auto level = SelectLevel(index, TileMetresPerPixel(bounds, width));
  const auto sources = CoveringSourceTiles(index.levels[level], bounds);
  if (sources.empty())
    throw std::runtime_error("Outside the radar composite");

  progress.SetProgressRange(unsigned(sources.size()));

  unsigned done = 0;
  for (const auto source : sources) {
    co_await composite.FetchTile(level, source, curl);
    progress.SetProgressPosition(++done);
  }

  auto path = AllocatedPath::Build(MakeCacheDirectory("opera"), "area.png");
  composite.Render(level, bounds, width, height, path);

  co_return std::move(path);
}
