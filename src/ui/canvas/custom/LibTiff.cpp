// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LibTiff.hpp"
#include "UncompressedImage.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <stdexcept>

#include <tiffio.h>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#endif

#ifdef USE_GEOTIFF
#include "Geo/Quadrilateral.hpp"
#include "Geo/ReferencedGrid.hpp"

#include <geotiff.h>
#include <geo_normalize.h>
#include <geovalues.h>
#include <xtiffio.h>

#include <proj.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#endif

static TIFF *
TiffOpen(Path path, const char *mode)
{
#ifdef USE_GEOTIFF
  XTIFFInitialize();
#endif

  return TIFFOpen(path.c_str(), mode);
}

class TiffLoader {
  TIFF *const tiff;

public:
  explicit TiffLoader(Path path)
    :tiff(TiffOpen(path, "r")) {
    if (tiff == nullptr)
      throw std::runtime_error("Failed to open TIFF file");
  }

  ~TiffLoader() {
    TIFFClose(tiff);
  }

  TIFF *Get() {
    return tiff;
  }

  void GetField(uint32_t tag, int &value_r) {
    TIFFGetField(tiff, tag, &value_r);
  }

  void RGBAImageBegin(TIFFRGBAImage &img) {
    char emsg[1024];
    if (!TIFFRGBAImageBegin(&img, tiff, 0, emsg))
      throw std::runtime_error(emsg);
  }
};

static UncompressedImage
LoadTiff(TIFFRGBAImage &img)
{
#ifdef ENABLE_OPENGL
  const unsigned max_size = OpenGL::max_texture_size;
#else
  constexpr unsigned max_size = 8192;
#endif

  if (img.width > max_size || img.height > max_size)
    throw std::runtime_error("TIFF file is too large");

  const std::size_t size = std::size_t(img.width) * img.height * 4;
  std::unique_ptr<uint8_t[]> data(new uint8_t[size]);
  uint32_t *data32 = (uint32_t *)(void *)data.get();

  if (!TIFFRGBAImageGet(&img, data32, img.width, img.height))
    throw std::runtime_error("Failed to copy TIFF data");

  return UncompressedImage(UncompressedImage::Format::RGBA, img.width * 4,
                           img.width, img.height, std::move(data), true);
}

static UncompressedImage
LoadTiff(TiffLoader &tiff)
{
  TIFFRGBAImage img;
  tiff.RGBAImageBegin(img);

  AtScopeExit(&img) { TIFFRGBAImageEnd(&img); };

  return LoadTiff(img);
}

UncompressedImage
LoadTiff(Path path)
{
  TiffLoader tiff(path);
  return LoadTiff(tiff);
}

#ifdef USE_GEOTIFF

/**
 * Converts GeoTIFF raster pixel coordinates to WGS84 geographic
 * coordinates. The raster->model step uses libgeotiff, the model->WGS84
 * step uses PROJ with the file's CRS so the geodetic datum shift is
 * applied. The EPSG code is used for the datum shift.
 * If the source CRS is not a usable EPSG code, it falls back to
 * GTIFProj4ToLatLong() (no datum shift) as a best effort.
 */
class GeoTiffTransform {
  GTIF &gtif;
  GTIFDefn &defn;

  PJ_CONTEXT *ctx = nullptr;
  PJ *pj = nullptr;

public:
  GeoTiffTransform(GTIF &_gtif, GTIFDefn &_defn) noexcept
    :gtif(_gtif), defn(_defn) {
    char src[32];
    if (defn.Model == ModelTypeProjected &&
        defn.PCS != KvUserDefined && defn.PCS != 0)
      snprintf(src, sizeof(src), "EPSG:%d", defn.PCS);
    else if (defn.Model == ModelTypeGeographic &&
             defn.GCS != KvUserDefined && defn.GCS != 0)
      snprintf(src, sizeof(src), "EPSG:%d", defn.GCS);
    else
      /* no usable EPSG code: leave pj==nullptr and fall back */
      return;

    ctx = proj_context_create();
    if (PJ *p = proj_create_crs_to_crs(ctx, src, "EPSG:4326", nullptr)) {
      /* normalise axis order to (easting/longitude, northing/latitude)
         on input and (longitude, latitude) on output */
      pj = proj_normalize_for_visualization(ctx, p);
      proj_destroy(p);
    }
  }

  ~GeoTiffTransform() noexcept {
    if (pj != nullptr)
      proj_destroy(pj);
    if (ctx != nullptr)
      proj_context_destroy(ctx);
  }

  GeoTiffTransform(const GeoTiffTransform &) = delete;
  GeoTiffTransform &operator=(const GeoTiffTransform &) = delete;

  GeoPoint PixelToGeoPoint(double x, double y) const noexcept {
    if (!GTIFImageToPCS(&gtif, &x, &y))
      return GeoPoint::Invalid();

    if (pj != nullptr) {
      const PJ_COORD c = proj_trans(pj, PJ_FWD, proj_coord(x, y, 0, 0));
      if (!std::isfinite(c.xy.x) || !std::isfinite(c.xy.y))
        return GeoPoint::Invalid();
      return GeoPoint(Angle::Degrees(c.xy.x), Angle::Degrees(c.xy.y));
    }

    /* fall back: invert the projection without a datum shift */
    if (defn.Model != ModelTypeGeographic &&
        !GTIFProj4ToLatLong(&defn, 1, &x, &y))
      return GeoPoint::Invalid();
    return GeoPoint(Angle::Degrees(x), Angle::Degrees(y));
  }
};

/**
 * Linear interpolation of longitude/latitude inside a quadrilateral -
 * the same bilinear map the renderer applies between four corners.
 * (s,t) are relative raster positions, 0..1 from the top-left.
 */
[[gnu::pure]]
static GeoPoint
BilinearCorner(const GeoQuadrilateral &q, double s, double t) noexcept
{
  const GeoPoint top = q.top_left + (q.top_right - q.top_left) * s;
  const GeoPoint bottom = q.bottom_left + (q.bottom_right - q.bottom_left) * s;
  return top + (bottom - top) * t;
}

/** Target georeferencing accuracy of the overlay mesh. */
static constexpr double kGridTargetMetres = 20;

/** Never subdivide an axis into more than this many cells. */
static constexpr unsigned kGridMaxCells = 64;

/**
 * Decide how nuch to subdivide the raster so that approximating each
 * cell by a bilinear quadrilateral stays within #kGridTargetMetres.
 *
 * Measures the worst deviation between the true transform
 * and the simple interpolation of the four corners. The error scales
 * with the square of the cell extent, so subdividing each axis by N
 * reduces it by N^2. Returns 1 when no subdivision is needed (e.g. a
 * geographic or equidistant-cylindrical raster, where the map is already
 * affine in lon/lat).
 */
[[gnu::pure]]
static unsigned
ChooseSubdivision(const GeoTiffTransform &transform,
                  const GeoQuadrilateral &corners,
                  int width, int height) noexcept
{
  double max_sag = 0;
  constexpr unsigned kSamples = 8;
  for (unsigned j = 1; j < kSamples; ++j) {
    for (unsigned i = 1; i < kSamples; ++i) {
      const double s = double(i) / kSamples, t = double(j) / kSamples;
      const GeoPoint truth = transform.PixelToGeoPoint(s * width, t * height);
      if (!truth.IsValid())
        continue;

      max_sag = std::max(max_sag,
                         truth.DistanceS(BilinearCorner(corners, s, t)));
    }
  }

  if (max_sag <= kGridTargetMetres)
    return 1;

  const auto n = (unsigned)std::ceil(std::sqrt(max_sag / kGridTargetMetres));
  return std::min(n, kGridMaxCells);
}

static GeoReferencedGrid
BuildGrid(const GeoTiffTransform &transform, int width, int height,
          const GeoQuadrilateral &corners)
{
  const unsigned n = ChooseSubdivision(transform, corners, width, height);
  if (n <= 1)
    return GeoReferencedGrid{corners};

  GeoReferencedGrid grid;
  grid.nx = grid.ny = n;
  grid.points.resize((std::size_t(n) + 1) * (std::size_t(n) + 1));

  for (unsigned j = 0; j <= n; ++j) {
    for (unsigned i = 0; i <= n; ++i) {
      const double s = double(i) / n, t = double(j) / n;
      GeoPoint p = transform.PixelToGeoPoint(s * width, t * height);
      if (!p.IsValid())
        /* keep the mesh well-formed if the transform fails at a node */
        p = BilinearCorner(corners, s, t);
      grid.points[j * (std::size_t(n) + 1) + i] = p;
    }
  }

  return grid;
}

std::pair<UncompressedImage, GeoReferencedGrid>
LoadGeoTiff(Path path)
{
  TiffLoader tiff(path);

  GeoReferencedGrid grid;

  {
    auto gtif = GTIFNew(tiff.Get());
    if (gtif == nullptr)
      throw std::runtime_error("Not a GeoTIFF file");

    AtScopeExit(gtif) { GTIFFree(gtif); };

    GTIFDefn defn;
    if (!GTIFGetDefn(gtif, &defn))
      throw std::runtime_error("Failed to parse GeoTIFF metadata");

    int width, height;
    tiff.GetField(TIFFTAG_IMAGEWIDTH, width);
    tiff.GetField(TIFFTAG_IMAGELENGTH, height);

    const GeoTiffTransform transform(*gtif, defn);

    GeoQuadrilateral corners;
    corners.top_left = transform.PixelToGeoPoint(0, 0);
    corners.top_right = transform.PixelToGeoPoint(width, 0);
    corners.bottom_left = transform.PixelToGeoPoint(0, height);
    corners.bottom_right = transform.PixelToGeoPoint(width, height);

    if (!corners.Check())
      throw std::runtime_error("Invalid GeoTIFF bounds");

    grid = BuildGrid(transform, width, height, corners);
  }

  return std::make_pair(LoadTiff(tiff), std::move(grid));
}

#endif
