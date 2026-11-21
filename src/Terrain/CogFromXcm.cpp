// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogFromXcm.hpp"
#include "TerrainBase.hpp"
#include "Height.hpp"
#include "Loader.hpp"
#include "RasterTile.hpp"
#include "RasterTileCache.hpp"
#include "thread/SharedMutex.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipReader.hpp"
#include "io/FileOutputStream.hxx"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Operation/SubOperationEnvironment.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#ifdef USE_LIBTIFF
#include "TiffGeoTags.hpp"
#ifdef USE_GEOTIFF
#include "Geo/GeoTIFFHeaders.hpp"
#endif
#endif

static AllocatedPath
CachedCogPath(Path xcm) noexcept
{
  const Path base = xcm != nullptr ? xcm.GetBase() : nullptr;
  if (base == nullptr)
    return nullptr;

  /* Name includes size and mtime so replacing the xcm does not
     reuse another map's convert. v3 writes TIFF overviews. */
  const auto sec =
    std::chrono::duration_cast<std::chrono::seconds>(
      File::GetLastModification(xcm).time_since_epoch()).count();
  return AllocatedPath::Build(GetCachePath(),
                              fmt::format("{}.{}.{}.v3.cog.tif",
                                          base.c_str(),
                                          File::GetSize(xcm),
                                          sec).c_str());
}

static void
ExtractZipMember(ZipArchive &archive, const char *name, Path dest)
{
  ZipReader in(archive.get(), name);
  Directory::Create(dest.GetParent());
  FileOutputStream os(dest, FileOutputStream::Mode::CREATE);
  std::array<std::byte, 65536> buf;
  while (true) {
    const auto n = in.Read(buf);
    if (n == 0)
      break;
    os.Write(std::span{buf.data(), n});
  }
  os.Commit();
}

#ifdef USE_LIBTIFF

static constexpr int16_t COG_NODATA = -32768;

static void
CopyCogTile(const RasterTile &tile, int16_t *dest,
            unsigned tw, unsigned th) noexcept
{
  std::fill(dest, dest + std::size_t(tw) * th, COG_NODATA);
  if (!tile.IsLoaded())
    return;

  const auto tsize = tile.buffer.GetSize();
  const TerrainHeight *src = tile.buffer.GetData();
  for (unsigned y = 0; y < tsize.y && y < th; ++y)
    for (unsigned x = 0; x < tsize.x && x < tw; ++x)
      dest[y * tw + x] = src[y * tsize.x + x].GetValue();
}

struct CogSink {
  TIFF *tf;
  RasterTileCache *rtc;
  int16_t *buf;
  uint32_t tw, th;
  unsigned grid_x;
  std::size_t buf_bytes;
  bool failed = false;
};

static void
OnConvertTile(void *ctx, unsigned index) noexcept
{
  auto &s = *static_cast<CogSink *>(ctx);
  if (s.failed || s.grid_x == 0)
    return;

  const unsigned tx = index % s.grid_x;
  const unsigned ty = index / s.grid_x;
  const auto grid = s.rtc->GetTileGridSize();
  if (tx >= grid.x || ty >= grid.y)
    return;

  RasterTile &tile = s.rtc->GetTile(tx, ty);
  CopyCogTile(tile, s.buf, s.tw, s.th);
  tile.Unload();
  tile.ClearRequest();

  const ttile_t tiff_index =
    TIFFComputeTile(s.tf, tx * s.tw, ty * s.th, 0, 0);
  if (TIFFWriteEncodedTile(s.tf, tiff_index, s.buf,
                           tmsize_t(s.buf_bytes)) < 0)
    s.failed = true;
}

static void
SetCogTiffFields(TIFF *tf, uint32_t width, uint32_t height,
                 uint32_t tw, uint32_t th,
                 double west, double north, double dx, double dy,
                 bool overview) noexcept
{
  if (overview)
    TIFFSetField(tf, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
  TIFFSetField(tf, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tf, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tf, TIFFTAG_TILEWIDTH, tw);
  TIFFSetField(tf, TIFFTAG_TILELENGTH, th);
  TIFFSetField(tf, TIFFTAG_SAMPLESPERPIXEL, uint16_t(1));
  TIFFSetField(tf, TIFFTAG_BITSPERSAMPLE, uint16_t(16));
  TIFFSetField(tf, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT);
  TIFFSetField(tf, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tf, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tf, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
  TIFFSetField(tf, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

  double tie[6] = {0, 0, 0, west, north, 0};
  double scale[3] = {dx, dy, 0};
  TIFFSetField(tf, TIFFTAG_GEOTIEPOINTS, 6, tie);
  TIFFSetField(tf, TIFFTAG_GEOPIXELSCALE, 3, scale);
}

static unsigned
OverviewTileCount(unsigned w, unsigned h, unsigned tw, unsigned th) noexcept
{
  unsigned n = 0;
  for (unsigned scale = 2;; scale *= 2) {
    w = (w + 1) / 2;
    h = (h + 1) / 2;
    if (w == 0 || h == 0)
      break;
    n += ((w + tw - 1) / tw) * ((h + th - 1) / th);
    if (w <= tw && h <= th)
      break;
    if (scale >= 256)
      break;
  }
  return n;
}

static void
WriteCogOverviews(Path dest, unsigned src_w, unsigned src_h,
                  uint32_t tw, uint32_t th,
                  double west, double north, double dx, double dy,
                  OperationEnvironment &operation, unsigned &done)
{
  const tmsize_t tile_bytes = tmsize_t(tw) * th * sizeof(int16_t);
  std::vector<int16_t> src(std::size_t(tw) * th);
  std::vector<int32_t> acc(std::size_t(tw) * th);
  std::vector<uint16_t> cnt(std::size_t(tw) * th);
  std::vector<uint16_t> water(std::size_t(tw) * th);
  std::vector<int16_t> dst(std::size_t(tw) * th);

  unsigned src_dir = 0;
  for (unsigned scale = 2;; scale *= 2) {
    const unsigned ow = (src_w + 1) / 2;
    const unsigned oh = (src_h + 1) / 2;
    if (ow == 0 || oh == 0)
      break;

    TIFF *in = TIFFOpen(dest.c_str(), "r");
    if (in == nullptr)
      throw std::runtime_error("Failed to reopen COG cache");
    AtScopeExit(in) { TIFFClose(in); };
    if (!TIFFSetDirectory(in, src_dir))
      throw std::runtime_error("Failed to read COG source directory");

    TIFF *out = TIFFOpen(dest.c_str(), "a");
    if (out == nullptr)
      throw std::runtime_error("Failed to append COG overview");
    AtScopeExit(out) { TIFFClose(out); };

    SetCogTiffFields(out, ow, oh, tw, th, west, north,
                     dx * scale, dy * scale, true);

    const unsigned src_tx = (src_w + tw - 1) / tw;
    const unsigned src_ty = (src_h + th - 1) / th;
    const unsigned ogx = (ow + tw - 1) / tw;
    const unsigned ogy = (oh + th - 1) / th;
    for (unsigned ty = 0; ty < ogy; ++ty) {
      for (unsigned tx = 0; tx < ogx; ++tx) {
        if (operation.IsCancelled())
          throw std::runtime_error("Cancelled");

        std::fill(acc.begin(), acc.end(), 0);
        std::fill(cnt.begin(), cnt.end(), 0);
        std::fill(water.begin(), water.end(), 0);

        for (unsigned sty = 0; sty < 2; ++sty) {
          for (unsigned stx = 0; stx < 2; ++stx) {
            const unsigned ftx = tx * 2 + stx;
            const unsigned fty = ty * 2 + sty;
            if (ftx >= src_tx || fty >= src_ty)
              continue;

            const ttile_t index =
              TIFFComputeTile(in, ftx * tw, fty * th, 0, 0);
            if (TIFFReadEncodedTile(in, index, src.data(),
                                    tile_bytes) < 0)
              throw std::runtime_error("Failed to read COG overview source");

            for (unsigned y = 0; y < th; ++y) {
              for (unsigned x = 0; x < tw; ++x) {
                const unsigned dx_i = (stx * tw + x) / 2;
                const unsigned dy_i = (sty * th + y) / 2;
                if (dx_i >= tw || dy_i >= th)
                  continue;
                const TerrainHeight h{src[y * tw + x]};
                if (h.IsInvalid())
                  continue;
                const unsigned di = dy_i * tw + dx_i;
                if (h.IsWater()) {
                  ++water[di];
                  continue;
                }
                acc[di] += src[y * tw + x];
                ++cnt[di];
              }
            }
          }
        }

        for (unsigned i = 0; i < dst.size(); ++i)
          dst[i] = cnt[i] > 0
            ? int16_t(acc[i] / int32_t(cnt[i]))
            : water[i] > 0 ? int16_t(-32767) : COG_NODATA;

        const ttile_t index = TIFFComputeTile(out, tx * tw, ty * th, 0, 0);
        if (TIFFWriteEncodedTile(out, index, dst.data(), tile_bytes) < 0)
          throw std::runtime_error("Failed to write COG overview");

        operation.SetProgressPosition(++done);
      }
    }

    if (!TIFFWriteDirectory(out))
      throw std::runtime_error("Failed to finalize COG overview");

    if (ow <= tw && oh <= th)
      break;
    if (scale >= 256)
      break;

    src_w = ow;
    src_h = oh;
    ++src_dir;
  }
}

static void
ConvertJp2ToCog(ZipArchive &archive, Path dest,
                OperationEnvironment &operation)
{
  const auto t0 = std::chrono::steady_clock::now();
  InstallCogTiffSupport();
  operation.SetProgressRange(1000);
  operation.SetProgressPosition(0);

  const AllocatedPath tmp{
    fmt::format("{}.tmp", dest.c_str()).c_str()};
  File::Delete(tmp);
  AtScopeExit(&tmp) { File::Delete(tmp); };

  RasterTileCache rtc;
  SharedMutex mutex;
  SubOperationEnvironment overview_env(operation, 0, 200);
  overview_env.SetProgressRange(200);
  TerrainLoader loader(mutex, rtc, true, false, overview_env);
  loader.LoadOverview(archive.get(), "terrain.jp2", "terrain.j2w");

  const auto size = rtc.GetSize();
  const auto grid = rtc.GetTileGridSize();
  const auto tile_px = rtc.GetTilePixelSize();
  const GeoBounds &bounds = rtc.GetBounds();
  if (size.x == 0 || size.y == 0 ||
      grid.x == 0 || grid.y == 0 ||
      tile_px.x == 0 || tile_px.y == 0 ||
      !bounds.IsValid())
    throw std::runtime_error("JP2 terrain has no size");

  Directory::Create(dest.GetParent());

#ifdef USE_GEOTIFF
  XTIFFInitialize();
#endif

  const uint32_t tw = tile_px.x;
  const uint32_t th = tile_px.y;
  const unsigned n_tiles = grid.x * grid.y;
  const unsigned n_ov = OverviewTileCount(size.x, size.y, tw, th);

  const double west = bounds.GetWest().Degrees();
  const double north = bounds.GetNorth().Degrees();
  const double dx = bounds.GetWidth().Degrees() / double(size.x);
  const double dy = bounds.GetHeight().Degrees() / double(size.y);

  {
    TIFF *tf = TIFFOpen(tmp.c_str(), "w");
    if (tf == nullptr)
      throw std::runtime_error("Failed to create COG cache");
    AtScopeExit(tf) { TIFFClose(tf); };

    SetCogTiffFields(tf, uint32_t(size.x), uint32_t(size.y), tw, th,
                     west, north, dx, dy, false);

#ifdef USE_GEOTIFF
    GTIF *gt = GTIFNew(tf);
    if (gt != nullptr) {
      GTIFKeySet(gt, GTModelTypeGeoKey, TYPE_SHORT, 1, ModelTypeGeographic);
      GTIFKeySet(gt, GTRasterTypeGeoKey, TYPE_SHORT, 1, RasterPixelIsArea);
      GTIFKeySet(gt, GeographicTypeGeoKey, TYPE_SHORT, 1, GCS_WGS_84);
      GTIFWriteKeys(gt);
      GTIFFree(gt);
    }
#endif

    SubOperationEnvironment tiles_env(operation, 200, 850);
    tiles_env.SetProgressRange(n_tiles);
    TerrainLoader tile_loader(mutex, rtc, false, false, tiles_env);
    std::vector<int16_t> tile_buf(std::size_t(tw) * th, COG_NODATA);

    bool any = false;
    for (unsigned k = 0; k < n_tiles; ++k) {
      RasterTile &tile = rtc.GetTile(k % grid.x, k / grid.x);
      if (!tile.IsDefined())
        continue;
      tile.SetRequest();
      any = true;
    }

    CogSink sink{
      tf, &rtc, tile_buf.data(), tw, th, grid.x,
      tile_buf.size() * sizeof(int16_t),
    };
    tile_loader.SetTileLoadedCallback(OnConvertTile, &sink);

    if (any)
      tile_loader.DecodeRequestedTiles(archive.get(), "terrain.jp2");

    if (sink.failed)
      throw std::runtime_error("Failed to write COG tile");

    if (!TIFFWriteDirectory(tf))
      throw std::runtime_error("Failed to finalize COG cache");
  }

  {
    SubOperationEnvironment ov_env(operation, 850, 1000);
    ov_env.SetProgressRange(n_ov > 0 ? n_ov : 1);
    unsigned ov_done = 0;
    WriteCogOverviews(tmp, size.x, size.y, tw, th,
                      west, north, dx, dy, ov_env, ov_done);
  }

  if (!File::Replace(tmp, dest))
    throw std::runtime_error("Failed to publish COG cache");

  const auto sec = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now() - t0).count();
  LogFmt("Converted terrain.jp2 to COG in {} s", sec);
}

#endif

std::unique_ptr<TerrainBase>
OpenXcmTerrain(FileCache *cache, Path map_path,
               OperationEnvironment &operation)
{
#ifdef USE_LIBTIFF
  const auto cog = CachedCogPath(map_path);
  if (cog != nullptr && File::Exists(cog)) {
    LogFormat("Opening cached COG terrain: %s", cog.c_str());
    return TerrainBase::OpenTerrain(cache, cog, operation);
  }
  if (cog == nullptr)
    throw std::runtime_error("Invalid map path");

  ZipArchive archive{map_path};
  Directory::Create(GetCachePath());

  if (archive.Exists("terrain.tif")) {
    LogFormat("Extracting terrain.tif from map");
    ExtractZipMember(archive, "terrain.tif", cog);
    return TerrainBase::OpenTerrain(cache, cog, operation);
  }

  if (archive.Exists("terrain.jp2")) {
    LogFormat("Converting terrain.jp2 to COG cache");
    try {
      ConvertJp2ToCog(archive, cog, operation);
    } catch (...) {
      File::Delete(cog);
      throw;
    }
    return TerrainBase::OpenTerrain(cache, cog, operation);
  }

  throw std::runtime_error("Map has no terrain.tif or terrain.jp2");
#else
  (void)cache;
  throw std::runtime_error("This build has no GeoTIFF terrain support");
#endif
}
