// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LibTiff.hpp"
#include "UncompressedImage.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <tiffio.h>

#ifdef USE_GEOTIFF
#include "Geo/Quadrilateral.hpp"
#include "Geo/GeoTIFFHeaders.hpp"
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

#if defined(_WIN32)
static UncompressedImage
BilinearUpscale(UncompressedImage &&src, unsigned scale)
{
  if (scale <= 1)
    return std::move(src);

  const unsigned src_width = src.GetWidth();
  const unsigned src_height = src.GetHeight();
  const unsigned dst_width = src_width * scale;
  const unsigned dst_height = src_height * scale;

  const auto format = src.GetFormat();
  unsigned bytes_per_pixel;
  switch (format) {
  case UncompressedImage::Format::RGBA:
    bytes_per_pixel = 4;
    break;

  case UncompressedImage::Format::RGB:
    bytes_per_pixel = 3;
    break;

  case UncompressedImage::Format::GRAY:
    bytes_per_pixel = 1;
    break;

  default:
    return std::move(src);
  }

  const auto src_pitch = src.GetPitch();
  const auto *src_data = (const uint8_t *)src.GetData();

  const std::size_t dst_pitch = std::size_t(dst_width) * bytes_per_pixel;
  auto dst_data = std::make_unique<uint8_t[]>(dst_pitch * dst_height);

  for (unsigned dst_y = 0; dst_y < dst_height; ++dst_y) {
    const float src_y = (float(dst_y) + 0.5f) / float(scale) - 0.5f;
    const int src_y_index = (int)std::floor(src_y);
    const float fy = src_y - float(src_y_index);

    const unsigned src_y0 = (unsigned)std::max(src_y_index, 0);
    const unsigned src_y1 = std::min((unsigned)(src_y_index + 1), src_height - 1);

    for (unsigned dst_x = 0; dst_x < dst_width; ++dst_x) {
      const float src_x = (float(dst_x) + 0.5f) / float(scale) - 0.5f;
      const int src_x_index = (int)std::floor(src_x);
      const float fx = src_x - float(src_x_index);

      const unsigned src_x0 = (unsigned)std::max(src_x_index, 0);
      const unsigned src_x1 = std::min((unsigned)(src_x_index + 1), src_width - 1);

      const uint8_t *p00 = src_data + src_y0 * src_pitch + src_x0 * bytes_per_pixel;
      const uint8_t *p10 = src_data + src_y0 * src_pitch + src_x1 * bytes_per_pixel;
      const uint8_t *p01 = src_data + src_y1 * src_pitch + src_x0 * bytes_per_pixel;
      const uint8_t *p11 = src_data + src_y1 * src_pitch + src_x1 * bytes_per_pixel;

      uint8_t *dst = dst_data.get() + dst_y * dst_pitch + dst_x * bytes_per_pixel;

      for (unsigned channel = 0; channel < bytes_per_pixel; ++channel) {
        const float value =
          float(p00[channel]) * (1.0f - fx) * (1.0f - fy) +
          float(p10[channel]) * fx * (1.0f - fy) +
          float(p01[channel]) * (1.0f - fx) * fy +
          float(p11[channel]) * fx * fy;
        const auto out_channel = (channel & 1u) ? channel : (channel + 2u) % 4u;
        dst[out_channel] = (uint8_t)(value + 0.5f);
      }
    }
  }

  return UncompressedImage(format, dst_pitch, dst_width, dst_height,
                           std::move(dst_data), src.IsFlipped());
}
#endif

static UncompressedImage
LoadTiff(TIFFRGBAImage &img)
{
  if (img.width > 8192 || img.height > 8192)
    throw std::runtime_error("TIFF file is too large");

  std::unique_ptr<uint8_t[]> data(new uint8_t[img.width * img.height * 4]);
  uint32_t *data32 = (uint32_t *)(void *)data.get();

  if (!TIFFRGBAImageGet(&img, data32, img.width, img.height))
    throw std::runtime_error("Failed to copy TIFF data");

#if defined(_WIN32)
  auto uncompressed = UncompressedImage(UncompressedImage::Format::RGBA,
                                        img.width * 4, img.width, img.height,
                                        std::move(data), true);
  return BilinearUpscale(std::move(uncompressed), 4);
#else
  return UncompressedImage(UncompressedImage::Format::RGBA, img.width * 4,
                           img.width, img.height, std::move(data), true);
#endif
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

static GeoPoint
TiffPixelToGeoPoint(GTIF &gtif, GTIFDefn &defn, double x, double y)
{
  if (!GTIFImageToPCS(&gtif, &x, &y))
    return GeoPoint::Invalid();

  if (defn.Model != ModelTypeGeographic &&
      !GTIFProj4ToLatLong(&defn, 1, &x, &y))
    return GeoPoint::Invalid();

  return GeoPoint(Angle::Degrees(x), Angle::Degrees(y));
}

std::pair<UncompressedImage, GeoQuadrilateral>
LoadGeoTiff(Path path)
{
  TiffLoader tiff(path);

  GeoQuadrilateral bounds;

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

    bounds.top_left = TiffPixelToGeoPoint(*gtif, defn, 0, 0);
    bounds.top_right = TiffPixelToGeoPoint(*gtif, defn, width, 0);
    bounds.bottom_left = TiffPixelToGeoPoint(*gtif, defn, 0, height);
    bounds.bottom_right = TiffPixelToGeoPoint(*gtif, defn, width, height);

    if (!bounds.Check())
      throw std::runtime_error("Invalid GeoTIFF bounds");
  }

  return std::make_pair(LoadTiff(tiff), bounds);
}

#endif
