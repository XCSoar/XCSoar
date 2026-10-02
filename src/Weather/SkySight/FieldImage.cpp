// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FieldImage.hpp"
#include "SkySightLimits.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#ifdef USE_GEOTIFF
#include "Geo/GeoTIFFHeaders.hpp"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <cstdarg>
#include <cstdio>
#include <new>
#include <stdexcept>
#include <string_view>
#include <vector>
#endif

namespace SkySight {

namespace {

constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();

/** Sample 0 is reserved for missing data. */
constexpr unsigned FIRST_SAMPLE = 1;
constexpr unsigned LAST_SAMPLE = 255;

} // namespace

bool
FieldQuantisation::IsValid() const noexcept
{
  return std::isfinite(minimum) && std::isfinite(maximum) &&
    maximum > minimum;
}

uint8_t
FieldQuantisation::Encode(float value) const noexcept
{
  if (!std::isfinite(value) || !IsValid())
    return 0;

  const float scaled = (value - minimum) / (maximum - minimum) *
    float(LAST_SAMPLE - FIRST_SAMPLE);
  const auto rounded = long(std::lround(scaled)) + long(FIRST_SAMPLE);
  return uint8_t(std::clamp(rounded, long(FIRST_SAMPLE), long(LAST_SAMPLE)));
}

float
FieldQuantisation::Decode(uint8_t sample) const noexcept
{
  if (sample < FIRST_SAMPLE || !IsValid())
    return MISSING;

  return minimum + float(sample - FIRST_SAMPLE) /
    float(LAST_SAMPLE - FIRST_SAMPLE) * (maximum - minimum);
}

FieldQuantisation
MeasureField(const ScalarField &field) noexcept
{
  FieldQuantisation result;
  if (field.empty())
    return result;

  float minimum = std::numeric_limits<float>::infinity();
  float maximum = -std::numeric_limits<float>::infinity();
  for (const float value : field.values) {
    if (!std::isfinite(value))
      continue;

    minimum = std::min(minimum, value);
    maximum = std::max(maximum, value);
  }

  if (minimum > maximum)
    return result;

  /* A constant field would divide by zero; widen it so every sample
     lands on the first step and still decodes to its own value. */
  if (minimum == maximum)
    maximum = std::nextafter(minimum, std::numeric_limits<float>::infinity());

  result.minimum = minimum;
  result.maximum = maximum;
  return result;
}

bool
GeoScalarField::IsValid() const noexcept
{
  return !field.empty() && north_west.IsValid() &&
    longitude_step > 0 && latitude_step > 0;
}

GeoBounds
GeoScalarField::GetBounds() const noexcept
{
  if (!IsValid())
    return GeoBounds::Invalid();

  /* GeoBounds takes the north-west and south-east corners, in that
     order; swapping them inverts the latitude range, and the overlay
     then never overlaps the screen. */
  return GeoBounds{
    north_west,
    GeoPoint{north_west.longitude +
               Angle::Degrees(longitude_step * field.width),
             north_west.latitude -
               Angle::Degrees(latitude_step * field.height)},
  };
}

IntPoint2D
GeoScalarField::ProjectClamped(GeoPoint p) const noexcept
{
  if (!IsValid())
    return {0, 0};

  const double x = (p.longitude - north_west.longitude).Degrees() /
    longitude_step;
  const double y = (north_west.latitude - p.latitude).Degrees() /
    latitude_step;

  return {
    int(std::clamp(std::floor(x), 0., double(field.width - 1))),
    int(std::clamp(std::floor(y), 0., double(field.height - 1))),
  };
}

GeoPoint
GeoScalarField::GetSampleCorner(unsigned x, unsigned y) const noexcept
{
  return GeoPoint{
    north_west.longitude + Angle::Degrees(longitude_step * x),
    north_west.latitude - Angle::Degrees(latitude_step * y),
  };
}

#ifdef USE_GEOTIFF

namespace {

/**
 * Identifies our private single-band layout.  Compared verbatim, so no
 * number ever has to be parsed back out of the file.
 */
constexpr std::string_view FIELD_IMAGE_MAGIC =
  "XCSoar SkySight forecast field v1";

#if TIFFLIB_VERSION > 20220520
void
LogTiffMessage(const char *module, const char *fmt, va_list ap)
{
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), fmt, ap);

  if (module != nullptr)
    LogFormat("%s: %s", module, buffer);
  else
    LogFormat("%s", buffer);
}

int
TiffErrorHandler(TIFF *, void *, const char *module, const char *fmt,
                 va_list ap)
{
  LogTiffMessage(module, fmt, ap);
  return 1;
}
#endif

TIFF *
OpenFieldTiff(Path path, const char *mode)
{
#if TIFFLIB_VERSION > 20220520
  TIFFOpenOptions *options = TIFFOpenOptionsAlloc();
  if (options == nullptr)
    throw std::bad_alloc();

  AtScopeExit(options) { TIFFOpenOptionsFree(options); };
  TIFFOpenOptionsSetErrorHandlerExtR(options, TiffErrorHandler, nullptr);
  TIFFOpenOptionsSetWarningHandlerExtR(options, TiffErrorHandler, nullptr);
  return XTIFFOpenExt(path.c_str(), mode, options);
#else
  return XTIFFOpen(path.c_str(), mode);
#endif
}

template<typename T>
[[nodiscard]] bool
GetScalarField(TIFF *tf, uint32_t tag, T &value) noexcept
{
  return TIFFGetField(tf, tag, &value) == 1;
}

} // namespace

void
WriteFieldImage(Path path, const GeoScalarField &source)
{
  if (!source.IsValid())
    throw std::runtime_error("SkySight forecast field is empty");

  const auto quantisation = MeasureField(source.field);
  if (!quantisation.IsValid())
    throw std::runtime_error("SkySight forecast field holds no data");

  {
  TIFF *tf = OpenFieldTiff(path, "w");
  if (tf == nullptr)
    throw std::runtime_error("SkySight field image open failed");

  AtScopeExit(tf) { TIFFClose(tf); };

  GTIF *gt = GTIFNew(tf);
  if (gt == nullptr)
    throw std::runtime_error("SkySight field image metadata init failed");

  AtScopeExit(gt) { GTIFFree(gt); };

  const double tie_points[6] = {
    0, 0, 0,
    source.north_west.longitude.Degrees(),
    source.north_west.latitude.Degrees(),
    0,
  };
  const double pixel_scale[3] = {
    source.longitude_step, source.latitude_step, 0,
  };

  TIFFSetField(tf, TIFFTAG_IMAGEWIDTH, source.field.width);
  TIFFSetField(tf, TIFFTAG_IMAGELENGTH, source.field.height);
  TIFFSetField(tf, TIFFTAG_SAMPLESPERPIXEL, uint16_t(1));
  TIFFSetField(tf, TIFFTAG_BITSPERSAMPLE, uint16_t(8));
  TIFFSetField(tf, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tf, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
  TIFFSetField(tf, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
  TIFFSetField(tf, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tf, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tf, TIFFTAG_IMAGEDESCRIPTION, FIELD_IMAGE_MAGIC.data());

  /* The sample-value tags carry the physical range the samples map onto;
     the magic above marks the file as ours, so no reader is misled. */
  TIFFSetField(tf, TIFFTAG_SMINSAMPLEVALUE, double(quantisation.minimum));
  TIFFSetField(tf, TIFFTAG_SMAXSAMPLEVALUE, double(quantisation.maximum));

  TIFFSetField(tf, TIFFTAG_GEOTIEPOINTS, 6, tie_points);
  TIFFSetField(tf, TIFFTAG_GEOPIXELSCALE, 3, pixel_scale);
  TIFFSetField(tf, TIFFTAG_ROWSPERSTRIP,
               TIFFDefaultStripSize(tf, source.field.width));

  GTIFKeySet(gt, GTModelTypeGeoKey, TYPE_SHORT, 1, ModelTypeGeographic);
  GTIFKeySet(gt, GTRasterTypeGeoKey, TYPE_SHORT, 1, RasterPixelIsArea);
  GTIFKeySet(gt, GeographicTypeGeoKey, TYPE_SHORT, 1, GCS_WGS_84);
  GTIFKeySet(gt, GeogLinearUnitsGeoKey, TYPE_SHORT, 1, Linear_Meter);
  GTIFKeySet(gt, GeogAngularUnitsGeoKey, TYPE_SHORT, 1, Angular_Degree);

  std::vector<uint8_t> row(source.field.width);
  for (unsigned y = 0; y < source.field.height; ++y) {
    for (unsigned x = 0; x < source.field.width; ++x)
      row[x] = quantisation.Encode(source.field.Get(x, y));

    if (TIFFWriteScanline(tf, row.data(), y, 0) != 1)
      throw std::runtime_error("SkySight field image write failed");
  }

  if (!GTIFWriteKeys(gt) || !TIFFWriteDirectory(tf))
    throw std::runtime_error("SkySight field image finalization failed");
  }

  /* Reopen to catch a truncated write before the file is published. */
  TIFF *tf = OpenFieldTiff(path, "r");
  if (tf == nullptr)
    throw std::runtime_error("SkySight field image validation failed");

  AtScopeExit(tf) { TIFFClose(tf); };

  const char *description = nullptr;
  uint32_t width = 0, height = 0;
  if (!GetScalarField(tf, TIFFTAG_IMAGEDESCRIPTION, description) ||
      description == nullptr ||
      std::string_view{description} != FIELD_IMAGE_MAGIC ||
      !GetScalarField(tf, TIFFTAG_IMAGEWIDTH, width) ||
      !GetScalarField(tf, TIFFTAG_IMAGELENGTH, height) ||
      width != source.field.width || height != source.field.height)
    throw std::runtime_error("SkySight field image validation failed");
}

GeoScalarField
ReadFieldImage(Path path)
{
  TIFF *tf = OpenFieldTiff(path, "r");
  if (tf == nullptr)
    throw std::runtime_error("SkySight field image open failed");

  AtScopeExit(tf) { TIFFClose(tf); };

  const char *description = nullptr;
  if (!GetScalarField(tf, TIFFTAG_IMAGEDESCRIPTION, description) ||
      description == nullptr ||
      std::string_view{description} != FIELD_IMAGE_MAGIC)
    throw std::runtime_error("Not a SkySight forecast field image");

  uint16_t samples_per_pixel = 0, bits_per_sample = 0;
  uint32_t width = 0, height = 0;
  if (!GetScalarField(tf, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel) ||
      !GetScalarField(tf, TIFFTAG_BITSPERSAMPLE, bits_per_sample) ||
      !GetScalarField(tf, TIFFTAG_IMAGEWIDTH, width) ||
      !GetScalarField(tf, TIFFTAG_IMAGELENGTH, height) ||
      samples_per_pixel != 1 || bits_per_sample != 8)
    throw std::runtime_error("SkySight forecast field image is malformed");

  if (!IsNetCdfGridSizeAllowed(height, width))
    throw ResourceLimitError("SkySight forecast field exceeds its size limit");

  FieldQuantisation quantisation;
  double minimum = 0, maximum = 0;
  if (!GetScalarField(tf, TIFFTAG_SMINSAMPLEVALUE, minimum) ||
      !GetScalarField(tf, TIFFTAG_SMAXSAMPLEVALUE, maximum))
    throw std::runtime_error("SkySight forecast field range is missing");

  quantisation.minimum = float(minimum);
  quantisation.maximum = float(maximum);
  if (!quantisation.IsValid())
    throw std::runtime_error("SkySight forecast field range is invalid");

  uint16_t tie_point_count = 0, pixel_scale_count = 0;
  const double *tie_points = nullptr;
  const double *pixel_scale = nullptr;
  if (TIFFGetField(tf, TIFFTAG_GEOTIEPOINTS, &tie_point_count,
                   &tie_points) != 1 ||
      TIFFGetField(tf, TIFFTAG_GEOPIXELSCALE, &pixel_scale_count,
                   &pixel_scale) != 1 ||
      tie_point_count < 6 || pixel_scale_count < 2)
    throw std::runtime_error("SkySight forecast field is not georeferenced");

  GeoScalarField result;
  result.north_west = GeoPoint{Angle::Degrees(tie_points[3]),
                               Angle::Degrees(tie_points[4])};
  result.longitude_step = pixel_scale[0];
  result.latitude_step = pixel_scale[1];
  result.field.width = width;
  result.field.height = height;
  result.field.values.resize(std::size_t(width) * height);

  std::vector<uint8_t> row(width);
  for (uint32_t y = 0; y < height; ++y) {
    if (TIFFReadScanline(tf, row.data(), y, 0) != 1)
      throw std::runtime_error("SkySight forecast field read failed");

    auto *out = &result.field.values[std::size_t(y) * width];
    for (uint32_t x = 0; x < width; ++x)
      out[x] = quantisation.Decode(row[x]);
  }

  if (!result.IsValid())
    throw std::runtime_error("SkySight forecast field is not georeferenced");

  return result;
}

#endif

} // namespace SkySight
