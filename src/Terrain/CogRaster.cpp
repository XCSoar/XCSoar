// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogRaster.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"

#ifdef USE_LIBTIFF
#include "TiffGeoTags.hpp"
#endif

#ifdef USE_GEOTIFF
#include "Geo/GeoTIFFHeaders.hpp"
#endif

#ifdef _WIN32
#include "system/UTF8Win32.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <vector>

struct TileCacheKey {
  unsigned level;
  unsigned tx;
  unsigned ty;

  bool operator<(const TileCacheKey &other) const noexcept {
    if (level != other.level)
      return level < other.level;
    if (ty != other.ty)
      return ty < other.ty;
    return tx < other.tx;
  }
};

using TileCache = std::map<TileCacheKey, std::vector<TerrainHeight>>;

/** Last tile hit so a scan line does not std::map::find every pixel. */
struct TileCursor {
  unsigned level = ~0u;
  unsigned x0 = 0, x1 = 0, y0 = 0, y1 = 0;
  unsigned tile_width = 1;
  const TerrainHeight *data = nullptr;
};

static unsigned
LevelForQuantisation(const std::vector<CogLevel> &levels,
                     unsigned quantisation) noexcept
{
  unsigned level = 0;
  const unsigned n = levels.size();
  if (quantisation > 1 && n > 1) {
    for (unsigned i = 1; i < n; ++i) {
      if (levels[i].scale <= quantisation)
        level = i;
      else
        break;
    }
  }
  return level;
}

#ifdef USE_LIBTIFF

static TIFFErrorHandler previous_tiff_warning;
#ifndef USE_GEOTIFF
static TIFFExtendProc parent_tiff_extender;

static void
CogTiffExtender(TIFF *tiff)
{
  static const TIFFFieldInfo info[] = {
    { TIFFTAG_GEOPIXELSCALE, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM,
      1, 1, const_cast<char *>("GeoPixelScale") },
    { TIFFTAG_GEOTIEPOINTS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM,
      1, 1, const_cast<char *>("GeoTiePoints") },
    { TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM,
      1, 0, const_cast<char *>("GDALNoDataValue") },
  };
  TIFFMergeFieldInfo(tiff, info, 3);
  if (parent_tiff_extender != nullptr)
    parent_tiff_extender(tiff);
}
#endif

void
InstallCogTiffSupport() noexcept
{
#ifdef USE_GEOTIFF
  static bool geotiff;
  if (!geotiff) {
    XTIFFInitialize();
    geotiff = true;
  }
#else
  static bool tags;
  if (!tags) {
    parent_tiff_extender = TIFFSetTagExtender(CogTiffExtender);
    tags = true;
  }
#endif
}

static void
CogTiffWarning(const char *module, const char *fmt, va_list ap)
{
  /* GeoTIFF + GDAL private tags: libtiff emits "Unknown field with
     tag …" for each overview. */
  if (fmt != nullptr && strstr(fmt, "nknown field") != nullptr)
    return;

  if (previous_tiff_warning != nullptr)
    previous_tiff_warning(module, fmt, ap);
}

static void
InstallTiffWarningFilter() noexcept
{
  static bool installed;
  if (installed)
    return;

  previous_tiff_warning = TIFFSetWarningHandler(CogTiffWarning);
  installed = true;
}

static TIFF *
OpenTiff(Path path)
{
  InstallTiffWarningFilter();
  InstallCogTiffSupport();

#ifdef _WIN32
  return TIFFOpenW(UTF8ToWide(path.c_str()).c_str(), "r");
#else
  return TIFFOpen(path.c_str(), "r");
#endif
}

static GeoBounds
ReadGeoBoundsFromTags(TIFF *tiff)
{
  uint32_t width = 0, height = 0;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
  if (width == 0 || height == 0)
    throw std::runtime_error("GeoTIFF has no size");

  uint16_t tie_count = 0, scale_count = 0;
  double *tie = nullptr;
  double *scale = nullptr;
  TIFFGetField(tiff, TIFFTAG_GEOTIEPOINTS, &tie_count, &tie);
  TIFFGetField(tiff, TIFFTAG_GEOPIXELSCALE, &scale_count, &scale);
  if (tie == nullptr || tie_count < 6 || scale == nullptr || scale_count < 2)
    throw std::runtime_error("GeoTIFF has no geographic tags");

  const double west = tie[3];
  const double north = tie[4];
  const GeoPoint top_left(Angle::Degrees(west), Angle::Degrees(north));
  const GeoPoint bottom_right(Angle::Degrees(west + scale[0] * width),
                              Angle::Degrees(north - scale[1] * height));
  if (!top_left.Check() || !bottom_right.Check())
    throw std::runtime_error("Invalid GeoTIFF bounds");

  return GeoBounds(top_left, bottom_right);
}

#ifdef USE_GEOTIFF
static GeoBounds
ReadGeoBounds(TIFF *tiff)
{
  auto gtif = GTIFNew(tiff);
  if (gtif == nullptr)
    return ReadGeoBoundsFromTags(tiff);

  AtScopeExit(gtif) { GTIFFree(gtif); };

  GTIFDefn defn;
  if (!GTIFGetDefn(gtif, &defn))
    return ReadGeoBoundsFromTags(tiff);

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
  const GeoPoint bottom_right = ToGeoPoint(width, height);

  if (!top_left.Check() || !bottom_right.Check())
    return ReadGeoBoundsFromTags(tiff);

  return GeoBounds(top_left, bottom_right);
}
#endif

[[gnu::pure]]
static bool
IsElevationDirectory(TIFF *tiff) noexcept
{
  if (!TIFFIsTiled(tiff))
    return false;

  uint16_t samples = 1, bits = 0, format = SAMPLEFORMAT_UINT;
  TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);
  TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits);
  TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &format);
  if (samples != 1)
    return false;
  if (bits == 16)
    return true;
  return bits == 32 && format == SAMPLEFORMAT_IEEEFP;
}

static CogLevel
ReadLevelInfo(TIFF *tiff, unsigned full_width)
{
  CogLevel level;

  uint32_t width, height;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);

  level.width = width;
  level.height = height;
  level.directory = TIFFCurrentDirectory(tiff);
  level.scale = (full_width > 0 && width > 0)
    ? std::max(1u, full_width / width)
    : 1;

  uint32_t tw, th;
  TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tw);
  TIFFGetField(tiff, TIFFTAG_TILELENGTH, &th);
  level.tile_width = tw;
  level.tile_height = th;

  level.tiles_x = (level.width + level.tile_width - 1) / level.tile_width;
  level.tiles_y = (level.height + level.tile_height - 1) / level.tile_height;

  char *nodata = nullptr;
  if (TIFFGetField(tiff, TIFFTAG_GDAL_NODATA, &nodata) &&
      nodata != nullptr && *nodata != '\0')
    level.nodata = int16_t(atoi(nodata));

  return level;
}

#endif

struct CogRaster::Impl {
#ifdef USE_LIBTIFF
  TIFF *tiff = nullptr;
#endif
  GeoBounds bounds;
  std::vector<CogLevel> levels;
  /* Protects tile_cache / pending.  ScanLine holds this. */
  mutable std::mutex mutex;
  /* TIFF ReadTile only.  Never hold mutex while waiting for this
     if the other thread might take mutex then this (deadlock). */
  mutable std::mutex file_mutex;
  mutable TileCache tile_cache;
  /* [level][ty * tiles_x + tx] → cached heights, or nullptr. */
  mutable std::vector<std::vector<const TerrainHeight *>> tile_ptr;
  mutable TileCursor last_cur, last_ov;
  mutable std::set<TileCacheKey> pending_internal_tiles;
  /* One screen is ~12 tiles; pan/zoom needs neighbours and the
     overview.  256×256 Int16 ≈ 128 kB, so 256 slots ≈ 32 MB.
     OpenTiffFile allows tiles up to 1024² (2 MB Int16); the byte
     cap keeps that case at 32 MB instead of 512 MB. */
  static constexpr size_t MAX_CACHED_TILES = 256;
  static constexpr size_t MAX_CACHED_BYTES = 32 * 1024 * 1024;
  size_t cached_bytes = 0;

  void CloseTiff() noexcept {
#ifdef USE_LIBTIFF
    if (tiff != nullptr) {
      TIFFClose(tiff);
      tiff = nullptr;
    }
#endif
    levels.clear();
    bounds.SetInvalid();
  }

  bool IsTiffOpen() const noexcept {
#ifdef USE_LIBTIFF
    return tiff != nullptr;
#else
    return false;
#endif
  }

#ifdef USE_LIBTIFF
  bool OpenTiffFile(Path path)
  {
    CloseTiff();

    tiff = OpenTiff(path);
    if (tiff == nullptr)
      return false;

    try {
#ifdef USE_GEOTIFF
      bounds = ReadGeoBounds(tiff);
#else
      bounds = ReadGeoBoundsFromTags(tiff);
#endif

      unsigned full_width = 0;
      do {
        if (!IsElevationDirectory(tiff))
          continue;

        uint32_t tw = 0, th = 0;
        TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tw);
        TIFFGetField(tiff, TIFFTAG_TILELENGTH, &th);
        if (tw == 0 || th == 0 || tw > 1024 || th > 1024)
          continue;

        CogLevel level = ReadLevelInfo(tiff, full_width);
        if (full_width == 0)
          full_width = level.width;
        levels.push_back(level);
      } while (TIFFReadDirectory(tiff));

      TIFFSetDirectory(tiff, 0);

      if (levels.empty()) {
        CloseTiff();
        return false;
      }

      return true;
    } catch (...) {
      CloseTiff();
      throw;
    }
  }

  bool ReadTile(unsigned level, unsigned tx, unsigned ty,
                TerrainHeight *buffer) noexcept
  {
    if (tiff == nullptr || level >= levels.size())
      return false;

    const CogLevel &lev = levels[level];
    if (tx >= lev.tiles_x || ty >= lev.tiles_y)
      return false;

    if (TIFFSetDirectory(tiff, lev.directory) == 0)
      return false;

    if (!IsElevationDirectory(tiff))
      return false;

    uint16_t bits_per_sample = 0;
    uint16_t sample_format = SAMPLEFORMAT_UINT;
    TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);

    const tmsize_t tile_size = TIFFTileSize(tiff);
    if (tile_size <= 0)
      return false;

    std::vector<uint8_t> raw_buffer(tile_size);

    const ttile_t tile_index = TIFFComputeTile(tiff,
                                               tx * lev.tile_width,
                                               ty * lev.tile_height,
                                               0, 0);

    if (TIFFReadEncodedTile(tiff, tile_index,
                            raw_buffer.data(), tile_size) < 0)
      return false;

    const unsigned tile_pixels = lev.tile_width * lev.tile_height;

    if (bits_per_sample == 16) {
      const auto *src = raw_buffer.data();
      for (unsigned i = 0; i < tile_pixels; ++i) {
        int16_t value;
        std::memcpy(&value, src + i * sizeof(int16_t), sizeof(int16_t));
        if (value == -32768 || value == lev.nodata)
          buffer[i] = TerrainHeight::Invalid();
        else
          buffer[i] = TerrainHeight(value);
      }
    } else if (bits_per_sample == 32 && sample_format == SAMPLEFORMAT_IEEEFP) {
      const auto *src = raw_buffer.data();
      for (unsigned i = 0; i < tile_pixels; ++i) {
        float value;
        std::memcpy(&value, src + i * sizeof(float), sizeof(float));
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
#else
  bool OpenTiffFile(Path) { return false; }
  bool ReadTile(unsigned, unsigned, unsigned, TerrainHeight *) noexcept {
    return false;
  }
#endif

  void ClearCache() noexcept {
    tile_cache.clear();
    pending_internal_tiles.clear();
    tile_ptr.clear();
    last_cur = {};
    last_ov = {};
    cached_bytes = 0;
  }

  void RebuildTilePtr() noexcept {
    last_cur = {};
    last_ov = {};
    tile_ptr.clear();
    const unsigned n = levels.size();
    tile_ptr.resize(n);
    for (unsigned i = 0; i < n; ++i) {
      const CogLevel &lev = levels[i];
      tile_ptr[i].assign(size_t(lev.tiles_x) * size_t(lev.tiles_y),
                         nullptr);
    }
  }

  void SetPtr(const TileCacheKey &key, const TerrainHeight *p) noexcept {
    if (key.level >= tile_ptr.size())
      return;
    const CogLevel &lev = levels[key.level];
    const size_t i = size_t(key.ty) * lev.tiles_x + key.tx;
    if (i < tile_ptr[key.level].size())
      tile_ptr[key.level][i] = p;
  }

  void EvictOne() noexcept {
    /* Keep the level ScanTerrain is using.  Evicting it made
       ScanLine fall back to the overview for a frame. */
    auto it = tile_cache.end();
    for (auto i = tile_cache.begin(); i != tile_cache.end(); ++i) {
      if (i->first.level != last_scan_level) {
        it = i;
        break;
      }
    }
    if (it == tile_cache.end())
      it = tile_cache.begin();
    const TerrainHeight *p = it->second.data();
    cached_bytes -= it->second.size() * sizeof(TerrainHeight);
    SetPtr(it->first, nullptr);
    if (last_cur.data == p)
      last_cur = {};
    if (last_ov.data == p)
      last_ov = {};
    tile_cache.erase(it);
  }

  void InsertTile(const TileCacheKey &key,
                  std::vector<TerrainHeight> &&tile_buffer) noexcept
  {
    if (tile_cache.find(key) != tile_cache.end())
      return;
    const size_t new_bytes = tile_buffer.size() * sizeof(TerrainHeight);
    while (!tile_cache.empty() &&
           (tile_cache.size() >= MAX_CACHED_TILES ||
            (cached_bytes + new_bytes > MAX_CACHED_BYTES &&
             new_bytes <= MAX_CACHED_BYTES)))
      EvictOne();
    auto &slot = tile_cache[key];
    slot = std::move(tile_buffer);
    cached_bytes += slot.size() * sizeof(TerrainHeight);
    SetPtr(key, slot.data());

    if (key.level < levels.size()) {
      const CogLevel &lev = levels[key.level];
      const unsigned x0 = key.tx * lev.tile_width;
      const unsigned y0 = key.ty * lev.tile_height;
      const auto refresh = [&](TileCursor &cur) {
        if (cur.level == key.level && cur.x0 == x0 && cur.y0 == y0)
          cur.data = slot.data();
      };
      refresh(last_cur);
      refresh(last_ov);
    }
  }

  bool DecodeOnePending(unsigned only_level) noexcept;
  unsigned QueueLevel(const GeoBounds &view, unsigned level,
                      unsigned extra_margin,
                      unsigned max_tiles = 0) noexcept;

  unsigned last_scan_level = 0;

  ~Impl() noexcept {
    CloseTiff();
  }

  void SeedCoarsestOverview() noexcept {
    unsigned overview, tile_pixels, max_tx, max_ty;
    {
      const std::lock_guard lock{mutex};
      const unsigned n = levels.size();
      if (n < 2)
        return;
      overview = n - 1;
      const CogLevel &lev = levels[overview];
      tile_pixels = lev.tile_width * lev.tile_height;
      max_tx = std::min(lev.tiles_x, 4u);
      max_ty = std::min(lev.tiles_y, 4u);
    }

    for (unsigned ty = 0; ty < max_ty; ++ty) {
      for (unsigned tx = 0; tx < max_tx; ++tx) {
        std::vector<TerrainHeight> tile_buffer(tile_pixels);
        bool ok;
        {
          const std::lock_guard file_lock{file_mutex};
          ok = ReadTile(overview, tx, ty, tile_buffer.data());
        }
        if (!ok)
          continue;
        const std::lock_guard lock{mutex};
        InsertTile({overview, tx, ty}, std::move(tile_buffer));
      }
    }
  }
};

CogRaster::CogRaster() noexcept
  :impl(std::make_unique<Impl>()) {}

CogRaster::~CogRaster() noexcept = default;

bool
CogRaster::IsCogFile(const char *path) noexcept
{
  if (path == nullptr || *path == '\0')
    return false;

  return StringEndsWithIgnoreCase(path, ".tif") ||
         StringEndsWithIgnoreCase(path, ".tiff");
}

bool
CogRaster::Open(Path path)
{
  {
    const std::lock_guard lock{impl->mutex};
    impl->ClearCache();
  }
  {
    const std::lock_guard file_lock{impl->file_mutex};
    if (!impl->OpenTiffFile(path))
      return false;
  }
  {
    const std::lock_guard lock{impl->mutex};
    impl->RebuildTilePtr();
  }
  impl->SeedCoarsestOverview();
  return true;
}

void
CogRaster::Close() noexcept
{
  {
    const std::lock_guard lock{impl->mutex};
    impl->ClearCache();
  }
  const std::lock_guard file_lock{impl->file_mutex};
  impl->CloseTiff();
}

bool
CogRaster::IsOpen() const noexcept
{
  return impl->IsTiffOpen();
}

const GeoBounds &
CogRaster::GetBounds() const noexcept
{
  return impl->bounds;
}

UnsignedPoint2D
CogRaster::GetSize() const noexcept
{
  if (impl->levels.empty())
    return {0, 0};
  return {impl->levels[0].width, impl->levels[0].height};
}

unsigned
CogRaster::GetLevelCount() const noexcept
{
  return impl->levels.size();
}

const CogLevel &
CogRaster::GetLevel(unsigned level) const noexcept
{
  return impl->levels[level];
}

UnsignedPoint2D
CogRaster::GetTileSize(unsigned level) const noexcept
{
  if (level >= impl->levels.size())
    return {0, 0};
  return {impl->levels[level].tile_width,
          impl->levels[level].tile_height};
}

using TilePtrGrid =
  std::vector<std::vector<const TerrainHeight *>>;

static bool
LoadTile(const TilePtrGrid &tile_ptr, TileCursor &cur, const CogLevel &lev,
         unsigned lvl, unsigned px, unsigned py) noexcept
{
  if (lev.tile_width == 0 || lev.tile_height == 0 ||
      px >= lev.width || py >= lev.height ||
      lvl >= tile_ptr.size()) {
    cur.level = ~0u;
    cur.data = nullptr;
    return false;
  }

  const unsigned tx = px / lev.tile_width;
  const unsigned ty = py / lev.tile_height;
  if (tx >= lev.tiles_x || ty >= lev.tiles_y) {
    cur.level = ~0u;
    cur.data = nullptr;
    return false;
  }

  cur.level = lvl;
  cur.tile_width = lev.tile_width;
  cur.x0 = tx * lev.tile_width;
  cur.x1 = std::min(cur.x0 + lev.tile_width, lev.width);
  cur.y0 = ty * lev.tile_height;
  cur.y1 = std::min(cur.y0 + lev.tile_height, lev.height);
  cur.data = tile_ptr[lvl][size_t(ty) * lev.tiles_x + tx];
  return cur.data != nullptr;
}

static TerrainHeight
PixelAt(const TilePtrGrid &tile_ptr, const TileCursor &cur,
        const CogLevel &lev, unsigned lvl,
        unsigned px, unsigned py) noexcept
{
  if (px >= lev.width)
    px = lev.width - 1;
  if (py >= lev.height)
    py = lev.height - 1;

  if (cur.data != nullptr && lvl == cur.level &&
      px >= cur.x0 && px < cur.x1 && py >= cur.y0 && py < cur.y1)
    return cur.data[(py - cur.y0) * cur.tile_width + (px - cur.x0)];

  TileCursor n{};
  if (!LoadTile(tile_ptr, n, lev, lvl, px, py) || n.data == nullptr)
    return TerrainHeight::Invalid();
  return n.data[(py - n.y0) * n.tile_width + (px - n.x0)];
}

/* Same 4-tap mix as RasterBuffer::GetInterpolated(), but the +1
   sample may live on the next TIFF tile.  Clamping to this tile
   made a hatch on every tile edge. */
[[gnu::hot]]
static TerrainHeight
InterpInTile(const TilePtrGrid &tile_ptr, const TileCursor &cur,
             const CogLevel &lev, unsigned lvl,
             unsigned px, unsigned py, unsigned ix, unsigned iy) noexcept
{
  const TerrainHeight h00 = PixelAt(tile_ptr, cur, lev, lvl, px, py);
  const TerrainHeight h10 = PixelAt(tile_ptr, cur, lev, lvl, px + 1, py);
  const TerrainHeight h01 = PixelAt(tile_ptr, cur, lev, lvl, px, py + 1);
  const TerrainHeight h11 = PixelAt(tile_ptr, cur, lev, lvl, px + 1, py + 1);
  if (h00.IsSpecial() || h10.IsSpecial() || h01.IsSpecial() ||
      h11.IsSpecial())
    return h00;

  const unsigned kx = 256 - ix;
  const unsigned ky = 256 - iy;
  return TerrainHeight(int16_t(
    (int32_t(h00.GetValue()) * int32_t(kx) * int32_t(ky) +
     int32_t(h10.GetValue()) * int32_t(ix) * int32_t(ky) +
     int32_t(h01.GetValue()) * int32_t(kx) * int32_t(iy) +
     int32_t(h11.GetValue()) * int32_t(ix) * int32_t(iy)) >> 16));
}

static TerrainHeight
SampleOverview(const TilePtrGrid &tile_ptr, TileCursor &ov_cur,
               const CogLevel &lev, const CogLevel &ov_lev,
               unsigned ov_level,
               unsigned px, unsigned py, unsigned ix, unsigned iy,
               bool interpolate) noexcept
{
  const double x =
    (double(px) + ix / 256.0) * double(ov_lev.width) / double(lev.width);
  const double y =
    (double(py) + iy / 256.0) * double(ov_lev.height) / double(lev.height);
  if (x < 0 || y < 0 ||
      x >= double(ov_lev.width) || y >= double(ov_lev.height))
    return TerrainHeight::Invalid();

  const unsigned opx = (unsigned)x;
  const unsigned opy = (unsigned)y;
  if (!LoadTile(tile_ptr, ov_cur, ov_lev, ov_level, opx, opy))
    return TerrainHeight::Invalid();
  if (!interpolate)
    return ov_cur.data[(opy - ov_cur.y0) * ov_cur.tile_width +
                       (opx - ov_cur.x0)];

  unsigned oix = (unsigned)((x - opx) * 256);
  unsigned oiy = (unsigned)((y - opy) * 256);
  if (oix > 255)
    oix = 255;
  if (oiy > 255)
    oiy = 255;
  return InterpInTile(tile_ptr, ov_cur, ov_lev, ov_level,
                      opx, opy, oix, oiy);
}

/* RasterBuffer::ScanHorizontalLine: west–east, mix in this function
   so -O0 does not call InterpInTile per sample.  Inner loop stays
   inside one tile so row/ddy are hoisted. */
[[gnu::hot]]
static void
ScanHorizontal(const TilePtrGrid &tile_ptr,
               TileCursor &cur, TileCursor &ov_cur,
               const CogLevel &lev, unsigned level,
               const CogLevel *ov_lev, unsigned ov_level,
               int64_t ax, int64_t bx, int64_t ay,
               TerrainHeight *buffer, unsigned size,
               bool interpolate) noexcept
{
  if (ay < 0) {
    std::fill(buffer, buffer + size, TerrainHeight::Invalid());
    return;
  }

  const unsigned py = unsigned(ay >> 8);
  const unsigned iy = unsigned(ay & 255);
  if (py >= lev.height) {
    std::fill(buffer, buffer + size, TerrainHeight::Invalid());
    return;
  }

  const unsigned ky = 256 - iy;
  const int64_t dx = bx - ax;
  const int64_t denom = size > 1 ? int64_t(size - 1) : 1;
  unsigned i = 0;
  while (i < size) {
    const int64_t fx = ax + int64_t(i) * dx / denom;
    if (fx < 0) {
      *buffer++ = TerrainHeight::Invalid();
      ++i;
      continue;
    }

    const unsigned px = unsigned(fx >> 8);
    const unsigned ix = unsigned(fx & 255);
    if (px >= lev.width) {
      *buffer++ = TerrainHeight::Invalid();
      ++i;
      continue;
    }

    if (level != cur.level || px < cur.x0 || px >= cur.x1 ||
        py < cur.y0 || py >= cur.y1) {
      if (!LoadTile(tile_ptr, cur, lev, level, px, py)) {
        *buffer++ = ov_lev != nullptr
          ? SampleOverview(tile_ptr, ov_cur, lev, *ov_lev, ov_level,
                           px, py, ix, iy, interpolate)
          : TerrainHeight::Invalid();
        ++i;
        continue;
      }
    } else if (cur.data == nullptr) {
      *buffer++ = ov_lev != nullptr
        ? SampleOverview(tile_ptr, ov_cur, lev, *ov_lev, ov_level,
                         px, py, ix, iy, interpolate)
        : TerrainHeight::Invalid();
      ++i;
      continue;
    }

    const unsigned tw = cur.tile_width;
    const TerrainHeight *const row =
      cur.data + (py - cur.y0) * tw;
    const unsigned ddy = (py + 1 < cur.y1) ? tw : 0u;
    const unsigned x1 = cur.x1;

    while (i < size) {
      const int64_t fx2 = ax + int64_t(i) * dx / denom;
      if (fx2 < 0)
        break;
      const unsigned px2 = unsigned(fx2 >> 8);
      if (px2 < cur.x0 || px2 >= x1)
        break;
      const unsigned ix2 = unsigned(fx2 & 255);
      const TerrainHeight *tm = row + (px2 - cur.x0);
      if (!interpolate) {
        *buffer++ = *tm;
        ++i;
        continue;
      }

      if (px2 + 1 >= x1 || py + 1 >= cur.y1) {
        *buffer++ = InterpInTile(tile_ptr, cur, lev, level,
                                 px2, py, ix2, iy);
        ++i;
        continue;
      }

      const unsigned ddx = 1u;
      if (tm->IsSpecial() || tm[ddx].IsSpecial() ||
          tm[ddy].IsSpecial() || tm[ddx + ddy].IsSpecial()) {
        *buffer++ = *tm;
        ++i;
        continue;
      }

      const unsigned kx = 256 - ix2;
      *buffer++ = TerrainHeight(int16_t(
        (int32_t(tm->GetValue()) * int32_t(kx) * int32_t(ky) +
         int32_t(tm[ddx].GetValue()) * int32_t(ix2) * int32_t(ky) +
         int32_t(tm[ddy].GetValue()) * int32_t(kx) * int32_t(iy) +
         int32_t(tm[ddx + ddy].GetValue()) * int32_t(ix2) *
           int32_t(iy)) >> 16));
      ++i;
    }
  }
}

TerrainHeight
CogRaster::GetHeight(const GeoPoint &location,
                     unsigned quantisation) const noexcept
{
  TerrainHeight h = TerrainHeight::Invalid();
  ScanLine(location, location, &h, 1, quantisation);
  return h;
}

void
CogRaster::ScanLine(const GeoPoint start, const GeoPoint end,
                    TerrainHeight *buffer, unsigned size,
                    unsigned quantisation) const noexcept
{
  if (size == 0)
    return;

  if (!impl->IsTiffOpen() || impl->levels.empty() ||
      !impl->bounds.IsValid()) {
    std::fill(buffer, buffer + size, TerrainHeight::Invalid());
    return;
  }

  const std::lock_guard lock{impl->mutex};
  const GeoBounds &bounds = impl->bounds;
  const double north = bounds.GetNorth().Native();
  const double west = bounds.GetWest().Native();
  const double b_h = bounds.GetHeight().Native();
  const double b_w = bounds.GetWidth().Native();
  if (b_h == 0 || b_w == 0) {
    std::fill(buffer, buffer + size, TerrainHeight::Invalid());
    return;
  }

  const unsigned level = LevelForQuantisation(impl->levels, quantisation);
  const CogLevel &lev = impl->levels[level];
  const unsigned n = impl->levels.size();
  const CogLevel *ov_lev = nullptr;
  unsigned ov_level = 0;
  if (n > 1 && level + 1 < n) {
    ov_level = n - 1;
    ov_lev = &impl->levels[ov_level];
  }

  const double x0 =
    ((start.longitude.Native() - west) / b_w) * double(lev.width);
  const double y0 =
    ((north - start.latitude.Native()) / b_h) * double(lev.height);
  const double x1 =
    ((end.longitude.Native() - west) / b_w) * double(lev.width);
  const double y1 =
    ((north - end.latitude.Native()) / b_h) * double(lev.height);

  const int64_t ax = (int64_t)(x0 * 256.0);
  const int64_t ay = (int64_t)(y0 * 256.0);
  const int64_t bx = (int64_t)(x1 * 256.0);
  const int64_t by = (int64_t)(y1 * 256.0);
  const int64_t dx = bx - ax;
  const int64_t dy = by - ay;
  /* RasterBuffer::ScanLine: no bilinear if a sample spans >~2 DEM
     pixels.  Fine units are 256 per pixel. */
  const bool interpolate =
    size > 1 &&
    (std::llabs(dx) + std::llabs(dy)) < int64_t(2 * size) * 256;

  if ((ay >> 8) == (by >> 8)) {
    ScanHorizontal(impl->tile_ptr, impl->last_cur, impl->last_ov,
                   lev, level, ov_lev, ov_level,
                   ax, bx, ay, buffer, size, interpolate);
    return;
  }

  TileCursor &cur = impl->last_cur;
  TileCursor &ov_cur = impl->last_ov;
  const int64_t denom = size > 1 ? int64_t(size - 1) : 1;
  for (unsigned i = 0; i < size; ++i) {
    const int64_t fx = ax + int64_t(i) * dx / denom;
    const int64_t fy = ay + int64_t(i) * dy / denom;
    if (fx < 0 || fy < 0) {
      *buffer++ = TerrainHeight::Invalid();
      continue;
    }

    const unsigned px = unsigned(fx >> 8);
    const unsigned py = unsigned(fy >> 8);
    const unsigned ix = unsigned(fx & 255);
    const unsigned iy = unsigned(fy & 255);
    if (px >= lev.width || py >= lev.height) {
      *buffer++ = TerrainHeight::Invalid();
      continue;
    }

    if (level != cur.level || px < cur.x0 || px >= cur.x1 ||
        py < cur.y0 || py >= cur.y1) {
      if (!LoadTile(impl->tile_ptr, cur, lev, level, px, py)) {
        *buffer++ = ov_lev != nullptr
          ? SampleOverview(impl->tile_ptr, ov_cur, lev, *ov_lev,
                           ov_level, px, py, ix, iy, interpolate)
          : TerrainHeight::Invalid();
        continue;
      }
    } else if (cur.data == nullptr) {
      *buffer++ = ov_lev != nullptr
        ? SampleOverview(impl->tile_ptr, ov_cur, lev, *ov_lev,
                         ov_level, px, py, ix, iy, interpolate)
        : TerrainHeight::Invalid();
      continue;
    }

    *buffer++ = interpolate
      ? InterpInTile(impl->tile_ptr, cur, lev, level, px, py, ix, iy)
      : cur.data[(py - cur.y0) * cur.tile_width + (px - cur.x0)];
  }
}

bool
CogRaster::Impl::DecodeOnePending(unsigned only_level) noexcept
{
  TileCacheKey key{};
  unsigned tile_pixels = 0;
  {
    const std::lock_guard lock{mutex};
    if (pending_internal_tiles.empty())
      return false;

    std::set<TileCacheKey>::iterator it;
    if (only_level != ~0u) {
      it = pending_internal_tiles.lower_bound({only_level, 0, 0});
      if (it == pending_internal_tiles.end() ||
          it->level != only_level)
        return false;
    } else {
      /* Coarsest overview first so a layer is visible before
         512×512 internals finish. */
      it = pending_internal_tiles.end();
      --it;
    }
    key = *it;
    pending_internal_tiles.erase(it);

    if (key.level < tile_ptr.size()) {
      const CogLevel &lev = levels[key.level];
      const size_t idx = size_t(key.ty) * lev.tiles_x + key.tx;
      if (idx < tile_ptr[key.level].size() &&
          tile_ptr[key.level][idx] != nullptr)
        return true;
    }

    if (key.level >= levels.size())
      return true;

    const CogLevel &lev = levels[key.level];
    if (key.tx >= lev.tiles_x || key.ty >= lev.tiles_y)
      return true;

    tile_pixels = lev.tile_width * lev.tile_height;
  }

  std::vector<TerrainHeight> tile_buffer(tile_pixels);
  bool ok;
  {
    const std::lock_guard file_lock{file_mutex};
    ok = ReadTile(key.level, key.tx, key.ty,
                       tile_buffer.data());
  }
  if (!ok)
    return true;

  const std::lock_guard lock{mutex};
  if (!IsTiffOpen())
    return true;
  InsertTile(key, std::move(tile_buffer));
  return true;
}

unsigned
CogRaster::Impl::QueueLevel(const GeoBounds &view, unsigned level,
                            unsigned extra_margin,
                            unsigned max_tiles) noexcept
{
  if (level >= levels.size())
    return 0;

  const CogLevel &lev = levels[level];
  const GeoBounds &file_bounds = bounds;
  if (!file_bounds.IsValid() || !view.IsValid() ||
      lev.tile_width == 0 || lev.tile_height == 0 ||
      lev.tiles_x == 0 || lev.tiles_y == 0)
    return 0;

  GeoBounds clipped = view;
  if (!clipped.IntersectWith(file_bounds))
    return 0;

  const double file_h = file_bounds.GetHeight().Degrees();
  const double file_w = file_bounds.GetWidth().Degrees();
  if (file_h == 0 || file_w == 0)
    return 0;

  const double lat_min =
    (file_bounds.GetNorth().Degrees() - clipped.GetNorth().Degrees()) /
    file_h;
  const double lat_max =
    (file_bounds.GetNorth().Degrees() - clipped.GetSouth().Degrees()) /
    file_h;
  const double lon_min =
    (clipped.GetWest().Degrees() - file_bounds.GetWest().Degrees()) /
    file_w;
  const double lon_max =
    (clipped.GetEast().Degrees() - file_bounds.GetWest().Degrees()) /
    file_w;

  const int px_min = std::max(0, static_cast<int>(lon_min * lev.width));
  const int px_max = std::min(static_cast<int>(lev.width) - 1,
                              static_cast<int>(lon_max * lev.width));
  const int py_min = std::max(0, static_cast<int>(lat_min * lev.height));
  const int py_max = std::min(static_cast<int>(lev.height) - 1,
                              static_cast<int>(lat_max * lev.height));
  if (px_max < px_min || py_max < py_min)
    return 0;

  const unsigned tx_min = px_min / lev.tile_width;
  const unsigned tx_max = px_max / lev.tile_width;
  const unsigned ty_min = py_min / lev.tile_height;
  const unsigned ty_max = py_max / lev.tile_height;

  const unsigned tx0 = tx_min > extra_margin
    ? tx_min - extra_margin : 0;
  const unsigned tx1 = std::min(tx_max + extra_margin, lev.tiles_x - 1);
  const unsigned ty0 = ty_min > extra_margin
    ? ty_min - extra_margin : 0;
  const unsigned ty1 = std::min(ty_max + extra_margin, lev.tiles_y - 1);

  /* Skip a zoom-in prefetch that would swamp the 256-tile cache. */
  if (max_tiles > 0 &&
      (tx1 - tx0 + 1) * (ty1 - ty0 + 1) > max_tiles)
    return 0;

  unsigned queued = 0;
  for (unsigned ty = ty0; ty <= ty1 && ty < lev.tiles_y; ++ty) {
    for (unsigned tx = tx0; tx <= tx1 && tx < lev.tiles_x; ++tx) {
      TileCacheKey key{level, tx, ty};
      if (level < tile_ptr.size()) {
        const size_t idx = size_t(ty) * lev.tiles_x + tx;
        if (idx < tile_ptr[level].size() &&
            tile_ptr[level][idx] != nullptr)
          continue;
      }
      if (pending_internal_tiles.find(key) !=
          pending_internal_tiles.end())
        continue;

      pending_internal_tiles.insert(key);
      ++queued;
    }
  }
  return queued;
}

unsigned
CogRaster::QueueInternalTiles(const GeoBounds &bounds,
                              unsigned quantisation) const noexcept
{
  if (!impl->IsTiffOpen() || impl->levels.size() == 0 ||
      !bounds.IsValid() || !impl->bounds.IsValid())
    return 0;

  unsigned level = 0;
  unsigned queued = 0;
  {
    const std::lock_guard lock{impl->mutex};
    const unsigned n = impl->levels.size();
    level = LevelForQuantisation(impl->levels, quantisation);
    impl->last_scan_level = level;
    queued = impl->QueueLevel(bounds, level, 1);
    const unsigned overview = n - 1;
    if (overview != level)
      queued += impl->QueueLevel(bounds, overview, 0);
  }

  /* Decode the scan level now so this Generate is a complete
     picture. */
  if (queued > 0)
    while (impl->DecodeOnePending(level)) {}

  /* Pan: one extra ring.  Zoom: neighbour pyramid levels.  Decoded
     by UpdateTiles / TerrainThread into the same tile cache. */
  {
    const std::lock_guard lock{impl->mutex};
    const unsigned n = impl->levels.size();
    impl->QueueLevel(bounds, level, 2);
    if (level + 1 < n)
      impl->QueueLevel(bounds, level + 1, 1);
    if (level > 0)
      impl->QueueLevel(bounds, level - 1, 0, 64);
  }

  return queued;
}

bool
CogRaster::ProcessScanTile() noexcept
{
  unsigned scan_level;
  {
    const std::lock_guard lock{impl->mutex};
    scan_level = impl->last_scan_level;
  }
  return impl->DecodeOnePending(scan_level);
}

bool
CogRaster::ProcessOneInternalTile() noexcept
{
  unsigned scan_level;
  {
    const std::lock_guard lock{impl->mutex};
    scan_level = impl->last_scan_level;
  }
  if (impl->DecodeOnePending(scan_level))
    return true;
  return impl->DecodeOnePending(~0u);
}

bool
CogRaster::HasPendingInternalTiles() const noexcept
{
  const std::lock_guard lock{impl->mutex};
  return !impl->pending_internal_tiles.empty();
}
