// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogLoader.hpp"
#include "RasterTileCache.hpp"
#include "RasterTraits.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"
#include "LogFile.hpp"

#include <tiffio.h>

#ifdef USE_GEOTIFF
#include "Geo/GeoTIFFHeaders.hpp"
#endif

#ifdef _WIN32
#include "system/UTF8Win32.hpp"
#endif

#include <algorithm>
#include <stdexcept>
#include <cstring>

struct CogLoader::Impl {
  TIFF *tiff = nullptr;
  GeoBounds bounds;
  std::vector<CogLevel> levels;

  ~Impl() noexcept {
    Close();
  }

  void Close() noexcept {
    if (tiff != nullptr) {
      TIFFClose(tiff);
      tiff = nullptr;
    }
    levels.clear();
    bounds.SetInvalid();
  }
};

CogLoader::CogLoader() noexcept
  :impl(std::make_unique<Impl>()) {}

CogLoader::~CogLoader() noexcept = default;

bool
CogLoader::IsCogFile(const char *path) noexcept
{
  return StringEndsWithIgnoreCase(path, ".tif") ||
         StringEndsWithIgnoreCase(path, ".tiff");
}

static TIFF *
OpenTiff(Path path)
{
#ifdef USE_GEOTIFF
  XTIFFInitialize();
#endif

#ifdef _WIN32
  return TIFFOpenW(UTF8ToWide(path.c_str()).c_str(), "r");
#else
  return TIFFOpen(path.c_str(), "r");
#endif
}

#ifdef USE_GEOTIFF
static GeoBounds
ReadGeoBounds(TIFF *tiff)
{
  auto gtif = GTIFNew(tiff);
  if (gtif == nullptr)
    throw std::runtime_error("Not a GeoTIFF file");

  AtScopeExit(gtif) { GTIFFree(gtif); };

  GTIFDefn defn;
  if (!GTIFGetDefn(gtif, &defn))
    throw std::runtime_error("Failed to parse GeoTIFF metadata");

  uint32_t width, height;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);

  auto ToGeoPoint = [&](double x, double y) -> GeoPoint {
    if (!GTIFImageToPCS(gtif, &x, &y))
      return GeoPoint::Invalid();

    if (defn.Model != ModelTypeGeographic &&
        !GTIFProj4ToLatLong(&defn, 1, &x, &y))
      return GeoPoint::Invalid();

    return GeoPoint(Angle::Degrees(x), Angle::Degrees(y));
  };

  const GeoPoint top_left = ToGeoPoint(0, 0);
  const GeoPoint top_right = ToGeoPoint(width, 0);
  const GeoPoint bottom_left = ToGeoPoint(0, height);
  const GeoPoint bottom_right = ToGeoPoint(width, height);

  if (!top_left.IsValid() || !bottom_right.IsValid())
    throw std::runtime_error("Invalid GeoTIFF bounds");

  return GeoBounds(top_left, bottom_right);
}
#endif

static CogLevel
ReadLevelInfo(TIFF *tiff, unsigned scale)
{
  CogLevel level;

  uint32_t width, height;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);

  level.width = width;
  level.height = height;
  level.scale = scale;

  if (TIFFIsTiled(tiff)) {
    uint32_t tw, th;
    TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tiff, TIFFTAG_TILELENGTH, &th);
    level.tile_width = tw;
    level.tile_height = th;
  } else {
    level.tile_width = width;
    level.tile_height = 1;
  }

  level.tiles_x = (level.width + level.tile_width - 1) / level.tile_width;
  level.tiles_y = (level.height + level.tile_height - 1) / level.tile_height;

  return level;
}

bool
CogLoader::Open(Path path)
{
  impl->Close();

  impl->tiff = OpenTiff(path);
  if (impl->tiff == nullptr)
    return false;

  try {
#ifdef USE_GEOTIFF
    impl->bounds = ReadGeoBounds(impl->tiff);
#else
    impl->bounds.SetInvalid();
#endif

    unsigned scale = 1;
    do {
      impl->levels.push_back(ReadLevelInfo(impl->tiff, scale));
      scale *= 2;
    } while (TIFFReadDirectory(impl->tiff));

    TIFFSetDirectory(impl->tiff, 0);

    if (impl->levels.empty()) {
      impl->Close();
      return false;
    }

    return true;
  } catch (...) {
    impl->Close();
    throw;
  }
}

bool
CogLoader::Open(struct zzip_dir *dir, const char *path)
{
  (void)dir;
  return Open(Path(path));
}

void
CogLoader::Close() noexcept
{
  impl->Close();
}

bool
CogLoader::IsOpen() const noexcept
{
  return impl->tiff != nullptr;
}

const GeoBounds &
CogLoader::GetBounds() const noexcept
{
  return impl->bounds;
}

UnsignedPoint2D
CogLoader::GetSize() const noexcept
{
  if (impl->levels.empty())
    return {0, 0};
  return {impl->levels[0].width, impl->levels[0].height};
}

unsigned
CogLoader::GetLevelCount() const noexcept
{
  return impl->levels.size();
}

const CogLevel &
CogLoader::GetLevel(unsigned level) const noexcept
{
  return impl->levels[level];
}

unsigned
CogLoader::SelectLevel(double meters_per_pixel) const noexcept
{
  if (impl->levels.empty())
    return 0;

#ifdef USE_GEOTIFF
  if (!impl->bounds.IsValid())
    return 0;

  const double terrain_meters_per_pixel =
    impl->bounds.GetHeight().IsPositive()
    ? impl->bounds.GetHeight().Degrees() * 111000.0 / impl->levels[0].height
    : 90.0;

  const double target_ratio = meters_per_pixel / terrain_meters_per_pixel;

  for (unsigned i = 0; i < impl->levels.size(); ++i) {
    if (impl->levels[i].scale >= target_ratio)
      return i;
  }
#endif

  return impl->levels.size() - 1;
}

UnsignedPoint2D
CogLoader::GetTileSize(unsigned level) const noexcept
{
  if (level >= impl->levels.size())
    return {0, 0};
  return {impl->levels[level].tile_width, impl->levels[level].tile_height};
}

bool
CogLoader::ReadTile(unsigned level, unsigned tx, unsigned ty,
                    TerrainHeight *buffer) noexcept
{
  if (!IsOpen() || level >= impl->levels.size())
    return false;

  const CogLevel &lev = impl->levels[level];
  if (tx >= lev.tiles_x || ty >= lev.tiles_y)
    return false;

  if (TIFFSetDirectory(impl->tiff, level) == 0)
    return false;

  if (!TIFFIsTiled(impl->tiff)) {
    return false;
  }

  uint16_t bits_per_sample = 0;
  uint16_t sample_format = SAMPLEFORMAT_UINT;
  TIFFGetField(impl->tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
  TIFFGetField(impl->tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);

  const tmsize_t tile_size = TIFFTileSize(impl->tiff);
  if (tile_size <= 0)
    return false;

  std::vector<uint8_t> raw_buffer(tile_size);

  const ttile_t tile_index = TIFFComputeTile(impl->tiff,
                                             tx * lev.tile_width,
                                             ty * lev.tile_height,
                                             0, 0);

  if (TIFFReadEncodedTile(impl->tiff, tile_index,
                          raw_buffer.data(), tile_size) < 0)
    return false;

  const unsigned tile_pixels = lev.tile_width * lev.tile_height;

  if (bits_per_sample == 16) {
    const int16_t *src = reinterpret_cast<const int16_t *>(raw_buffer.data());
    for (unsigned i = 0; i < tile_pixels; ++i) {
      int16_t value = src[i];
      if (value == -32768)
        buffer[i] = TerrainHeight::Invalid();
      else
        buffer[i] = TerrainHeight(value);
    }
  } else if (bits_per_sample == 32 && sample_format == SAMPLEFORMAT_IEEEFP) {
    const float *src = reinterpret_cast<const float *>(raw_buffer.data());
    for (unsigned i = 0; i < tile_pixels; ++i) {
      float value = src[i];
      if (std::isnan(value) || value < -32767 || value > 32767)
        buffer[i] = TerrainHeight::Invalid();
      else
        buffer[i] = TerrainHeight(static_cast<int16_t>(value));
    }
  } else {
    std::fill_n(buffer, tile_pixels, TerrainHeight::Invalid());
    return false;
  }

  return true;
}

bool
IsCogTerrainFile(const char *filename) noexcept
{
  return CogLoader::IsCogFile(filename);
}

void
LoadCogTerrain(Path path, RasterTileCache &cache,
               OperationEnvironment &env)
{
  CogLoader loader;
  if (!loader.Open(path))
    throw std::runtime_error("Failed to open COG terrain file");

  env.SetProgressRange(100);
  env.SetProgressPosition(10);

  const UnsignedPoint2D size = loader.GetSize();
  if (size.x == 0 || size.y == 0)
    throw std::runtime_error("COG terrain has zero size");

#ifdef USE_GEOTIFF
  const GeoBounds &bounds = loader.GetBounds();
  if (!bounds.IsValid())
    throw std::runtime_error("COG terrain has no valid bounds");
  cache.SetBounds(bounds);
#endif

  const unsigned level_count = loader.GetLevelCount();
  if (level_count == 0)
    throw std::runtime_error("COG has no pyramid levels");

  unsigned overview_level = level_count - 1;
  const CogLevel &overview_lev = loader.GetLevel(overview_level);

  constexpr unsigned overview_shift = RasterTraits::OVERVIEW_BITS;
  const unsigned overview_width = (size.x + (1 << overview_shift) - 1) >> overview_shift;
  const unsigned overview_height = (size.y + (1 << overview_shift) - 1) >> overview_shift;

  const UnsignedPoint2D tile_size = loader.GetTileSize(0);
  const CogLevel &full_level = loader.GetLevel(0);

  cache.SetSize({size.x, size.y},
                {static_cast<uint_least16_t>(tile_size.x),
                 static_cast<uint_least16_t>(tile_size.y)},
                {full_level.tiles_x, full_level.tiles_y});

  env.SetProgressPosition(50);

  LogFmt("COG terrain: {}x{}, {} levels, tile {}x{}, {} tiles",
         size.x, size.y, level_count,
         tile_size.x, tile_size.y,
         full_level.tiles_x * full_level.tiles_y);

  env.SetProgressPosition(100);
}

void
LoadCogTerrain(struct zzip_dir *dir, const char *path,
               RasterTileCache &cache,
               OperationEnvironment &env)
{
  (void)dir;
  LoadCogTerrain(Path(path), cache, env);
}

void
UpdateCogTiles(struct zzip_dir *dir, const char *path,
               RasterTileCache &cache, SharedMutex &mutex,
               SignedRasterLocation center, unsigned radius)
{
  (void)dir;
  (void)path;
  (void)cache;
  (void)mutex;
  (void)center;
  (void)radius;
}
