// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightFileDecoder.hpp"

#include "LogFile.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "io/ZipArchive.hpp"
#include "io/ZipReader.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/zlib/GunzipReader.hxx"
#include "system/FileUtil.hpp"
#include "util/ScopeExit.hxx"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)
#include <netcdf.h>
#include "Geo/GeoTIFFHeaders.hpp"
#endif

namespace {

[[nodiscard]] static AllocatedPath
CopyPath(Path path)
{
  return AllocatedPath(path.c_str());
}

[[nodiscard]] static bool
HasForecastDataSuffix(std::string_view path) noexcept
{
  return path.ends_with(".nc") ||
    path.ends_with(".nc.min") ||
    path.ends_with(".tif") || path.ends_with(".tiff") ||
    path.ends_with(".tif.min") || path.ends_with(".tiff.min") ||
    path.ends_with(".png") ||
    path.ends_with(".png.min") ||
    path.ends_with(".jpg") || path.ends_with(".jpeg");
}

enum class ForecastPayloadType : uint8_t {
  Unknown,
  Zip,
  Gzip,
  NetCdf,
  Tiff,
  Png,
  Jpeg,
};

struct PreparedForecastPayload {
  AllocatedPath source_path;
  AllocatedPath cleanup_download_path;
  ForecastPayloadType type = ForecastPayloadType::Unknown;
};

[[nodiscard]] static bool
StartsWith(std::span<const std::byte> buffer,
           std::initializer_list<uint8_t> prefix) noexcept
{
  if (buffer.size() < prefix.size())
    return false;

  std::size_t index = 0;
  for (const auto value : prefix)
    if (buffer[index++] != std::byte{value})
      return false;

  return true;
}

[[nodiscard]] static std::span<const std::byte>
ReadMagic(Path path, std::span<std::byte> buffer) noexcept
{
  try {
    FileReader file(path);
    const auto nbytes = file.Read(buffer);
    return {buffer.data(), nbytes};
  } catch (...) {
    return {};
  }
}

[[nodiscard]] static ForecastPayloadType
DetectForecastPayloadType(Path path) noexcept
{
  std::array<std::byte, 16> buffer;
  const auto magic = ReadMagic(path, buffer);

  if (StartsWith(magic, {'P', 'K', 0x03, 0x04}) ||
      StartsWith(magic, {'P', 'K', 0x05, 0x06}) ||
      StartsWith(magic, {'P', 'K', 0x07, 0x08}))
    return ForecastPayloadType::Zip;

  if (StartsWith(magic, {0x1f, 0x8b}))
    return ForecastPayloadType::Gzip;

  if (StartsWith(magic, {'C', 'D', 'F', 0x01}) ||
      StartsWith(magic, {'C', 'D', 'F', 0x02}) ||
      StartsWith(magic, {0x89, 'H', 'D', 'F', 0x0d, 0x0a, 0x1a, 0x0a}))
    return ForecastPayloadType::NetCdf;

  if (StartsWith(magic, {'I', 'I', 0x2a, 0x00}) ||
      StartsWith(magic, {'M', 'M', 0x00, 0x2a}) ||
      StartsWith(magic, {'I', 'I', 0x2b, 0x00}) ||
      StartsWith(magic, {'M', 'M', 0x00, 0x2b}))
    return ForecastPayloadType::Tiff;

  if (StartsWith(magic, {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a}))
    return ForecastPayloadType::Png;

  if (StartsWith(magic, {0xff, 0xd8, 0xff}))
    return ForecastPayloadType::Jpeg;

  if (path.EndsWithIgnoreCase(".zip"))
    return ForecastPayloadType::Zip;

  if (path.EndsWithIgnoreCase(".min"))
    return ForecastPayloadType::Gzip;

  if (path.EndsWithIgnoreCase(".nc"))
    return ForecastPayloadType::NetCdf;

  if (path.EndsWithIgnoreCase(".tif") || path.EndsWithIgnoreCase(".tiff"))
    return ForecastPayloadType::Tiff;

  if (path.EndsWithIgnoreCase(".png"))
    return ForecastPayloadType::Png;

  if (path.EndsWithIgnoreCase(".jpg") || path.EndsWithIgnoreCase(".jpeg"))
    return ForecastPayloadType::Jpeg;

  return ForecastPayloadType::Unknown;
}

[[nodiscard]] static bool
IsDisplayReadyType(ForecastPayloadType type) noexcept
{
  return type == ForecastPayloadType::Tiff ||
    type == ForecastPayloadType::Png ||
    type == ForecastPayloadType::Jpeg;
}

[[nodiscard]] static const char *
GetPayloadSuffix(ForecastPayloadType type) noexcept
{
  switch (type) {
  case ForecastPayloadType::Tiff:
    return ".tif";

  case ForecastPayloadType::Png:
    return ".png";

  case ForecastPayloadType::Jpeg:
    return ".jpg";

  case ForecastPayloadType::NetCdf:
    return ".nc";

  default:
    return nullptr;
  }
}

[[nodiscard]] static AllocatedPath
ReplacePath(Path source_path, AllocatedPath target_path)
{
  if (target_path == source_path)
    return CopyPath(source_path);

  File::Delete(target_path);
  if (!File::Replace(source_path, target_path))
    throw std::runtime_error("Failed to normalise SkySight forecast payload suffix");

  return target_path;
}

[[nodiscard]] static AllocatedPath
GetNormalisedPayloadTarget(Path source_path, const char *suffix)
{
  if (!source_path.EndsWithIgnoreCase(".min"))
    return AllocatedPath(source_path.WithSuffix(suffix).c_str());

  std::string base{source_path.c_str()};
  base.resize(base.size() - 4);

  Path base_path{base.c_str()};
  if (base_path.EndsWithIgnoreCase(suffix))
    return AllocatedPath(base_path.c_str());

  return AllocatedPath(base_path.WithSuffix(suffix).c_str());
}

[[nodiscard]] static AllocatedPath
NormalisePayloadPath(Path source_path, ForecastPayloadType type)
{
  const auto *suffix = GetPayloadSuffix(type);
  if (suffix == nullptr)
    return CopyPath(source_path);

  return ReplacePath(source_path,
                     GetNormalisedPayloadTarget(source_path, suffix));
}

[[nodiscard]] static bool
NeedsNetCdfDecode(Path path) noexcept
{
  return path.EndsWithIgnoreCase(".nc");
}

static void
DeleteIfExists(Path path) noexcept
{
  if (File::Exists(path))
    File::Delete(path);
}

[[nodiscard]] static bool
NeedsGunzipForecastPayload(Path path) noexcept;

[[nodiscard]] static AllocatedPath
GetGunzipOutputPath(Path compressed_path);

static void
DeleteNetCdfDisplayArtifact(Path path) noexcept
{
  if (NeedsNetCdfDecode(path))
    DeleteIfExists(path.WithSuffix(".tif"));
}

static void
DeleteArchiveExtractionVariants(Path archive_path) noexcept
{
  DeleteIfExists(archive_path.WithSuffix(".min"));
  DeleteIfExists(archive_path.WithSuffix(".nc"));
  DeleteIfExists(archive_path.WithSuffix(".tif"));
  DeleteIfExists(archive_path.WithSuffix(".tiff"));
  DeleteIfExists(archive_path.WithSuffix(".png"));
  DeleteIfExists(archive_path.WithSuffix(".jpg"));
  DeleteIfExists(archive_path.WithSuffix(".jpeg"));
}

static void
DeletePreparedPayloadArtifacts(Path path) noexcept
{
  if (NeedsGunzipForecastPayload(path)) {
    const auto inflated_path = GetGunzipOutputPath(path);
    DeleteIfExists(inflated_path);
    DeleteNetCdfDisplayArtifact(inflated_path);
    return;
  }

  DeleteNetCdfDisplayArtifact(path);
}

static void
CopyReader(Reader &reader, OutputStream &output)
{
  while (true) {
    std::array<std::byte, 64 * 1024> buffer;
    const auto nbytes = reader.Read(buffer);
    if (nbytes == 0)
      break;

    output.Write(std::span<const std::byte>{buffer.data(), nbytes});
  }
}

[[nodiscard]] static bool
NeedsGunzipForecastPayload(Path path) noexcept
{
  return path.EndsWithIgnoreCase(".min");
}

[[nodiscard]] static AllocatedPath
GetGunzipOutputPath(Path compressed_path)
{
  std::string output_value{compressed_path.c_str()};
  if (compressed_path.EndsWithIgnoreCase(".min") && output_value.size() > 4)
    output_value.resize(output_value.size() - 4);

  if (Path{output_value.c_str()}.GetSuffix() == nullptr)
    return Path{output_value.c_str()}.WithSuffix(".nc");

  return AllocatedPath(output_value.c_str());
}

[[nodiscard]] static AllocatedPath
InflateForecastPayload(Path compressed_path)
{
  const auto output_path = GetGunzipOutputPath(compressed_path);
  if (File::Exists(output_path) &&
      File::GetLastModification(output_path) >=
        File::GetLastModification(compressed_path))
    return AllocatedPath(output_path.c_str());

  FileReader file(compressed_path);
  GunzipReader gunzip(file);
  FileOutputStream output(output_path);
  CopyReader(gunzip, output);
  output.Commit();
  return AllocatedPath(output_path.c_str());
}

[[nodiscard]] static AllocatedPath
ExtractArchiveEntry(Path archive_path)
{
  ZipArchive archive(archive_path);

  std::string fallback_entry_name;
  std::string entry_name;
  while (true) {
    entry_name = archive.NextName();
    if (entry_name.empty())
      break;

    if (entry_name.back() == '/')
      continue;

    if (fallback_entry_name.empty())
      fallback_entry_name = entry_name;

    if (HasForecastDataSuffix(entry_name))
      break;
  }

  if (entry_name.empty())
    entry_name = fallback_entry_name;

  if (entry_name.empty())
    throw std::runtime_error("SkySight forecast archive is empty");

  const auto suffix = Path{entry_name.c_str()}.GetSuffix();
  const auto output_path = suffix != nullptr
    ? archive_path.WithSuffix(suffix)
    : archive_path.WithSuffix(".payload");

  if (File::Exists(output_path))
    return AllocatedPath(output_path.c_str());

  ZipReader reader(archive.get(), entry_name.c_str());
  FileOutputStream output(output_path);
  std::array<std::byte, 64 * 1024> buffer;

  while (true) {
    const auto nbytes = reader.Read(buffer);
    if (nbytes == 0)
      break;

    output.Write(std::span<const std::byte>{buffer.data(), nbytes});
  }

  output.Commit();
  return AllocatedPath(output_path.c_str());
}

[[nodiscard]] static PreparedForecastPayload
PrepareForecastPayload(Path path)
{
  PreparedForecastPayload payload{
    AllocatedPath(path.c_str()),
    {},
    DetectForecastPayloadType(path),
  };

  if (payload.type == ForecastPayloadType::Zip) {
    payload.source_path = ExtractArchiveEntry(payload.source_path);
    payload.type = DetectForecastPayloadType(payload.source_path);
  }

  if (payload.type == ForecastPayloadType::Gzip) {
    payload.source_path = InflateForecastPayload(payload.source_path);
    payload.type = DetectForecastPayloadType(payload.source_path);
  }

  if (payload.type == ForecastPayloadType::NetCdf ||
      IsDisplayReadyType(payload.type))
    payload.source_path = NormalisePayloadPath(payload.source_path,
                                              payload.type);

  if (payload.source_path != path)
    payload.cleanup_download_path = AllocatedPath(path.c_str());

  return payload;
}

[[nodiscard]] static SkySightPreparedData
MakeDisplayReadyData(Path path)
{
  return {
    SkySightPreparedDataKind::DisplayReady,
    CopyPath(path),
    CopyPath(path),
  };
}

[[nodiscard]] static SkySightPreparedData
PrepareNetCdfPayload(PreparedForecastPayload payload)
{
  auto display_path = payload.source_path.WithSuffix(".tif");
  auto cleanup_source_path = CopyPath(payload.source_path);

  if (File::Exists(display_path) &&
      File::GetLastModification(display_path) >=
        File::GetLastModification(payload.source_path)) {
    DeleteIfExists(payload.source_path);
    DeleteIfExists(payload.cleanup_download_path);
    return MakeDisplayReadyData(display_path);
  }

  SkySightPreparedData prepared;
  prepared.kind = SkySightPreparedDataKind::NeedsNetCdfDecode;
  prepared.source_path = std::move(payload.source_path);
  prepared.display_path = std::move(display_path);
  prepared.cleanup_source_path = std::move(cleanup_source_path);
  prepared.cleanup_download_path = std::move(payload.cleanup_download_path);
  return prepared;
}

#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)

[[maybe_unused]] static void
TiffErrorHandler(const char *module, const char *fmt, va_list ap)
{
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), fmt, ap);

  if (module != nullptr)
    LogFormat("%s: %s", module, buffer);
  else
    LogFormat("%s", buffer);
}

static void
ThrowNetCdfError(int status, const char *action)
{
  if (status != NC_NOERR)
    throw FmtRuntimeError("SkySight NetCDF {}: {}",
                          action, nc_strerror(status));
}

static double
GetOptionalDoubleAttribute(int file_id, int variable_id,
                           const char *name, double fallback)
{
  double value = fallback;
  const auto status = nc_get_att_double(file_id, variable_id, name, &value);
  if (status == NC_ENOTATT)
    return fallback;

  ThrowNetCdfError(status, name);
  return value;
}

static AllocatedPath
DecodeNetCdf(const SkySightPreparedData &prepared,
             std::string_view variable_name,
             const std::map<float, SkySight::LegendColor> &legend)
{
  if (legend.empty())
    throw std::runtime_error("SkySight legend is empty");

  File::Delete(prepared.display_path);

  int file_id = -1;
  ThrowNetCdfError(nc_open(prepared.source_path.c_str(), NC_NOWRITE, &file_id),
                   "open");
  AtScopeExit(file_id) { if (file_id >= 0) nc_close(file_id); };

  int lat_dim_id = -1, lon_dim_id = -1;
  ThrowNetCdfError(nc_inq_dimid(file_id, "lat", &lat_dim_id), "find lat dimension");
  ThrowNetCdfError(nc_inq_dimid(file_id, "lon", &lon_dim_id), "find lon dimension");

  size_t lat_size = 0, lon_size = 0;
  ThrowNetCdfError(nc_inq_dimlen(file_id, lat_dim_id, &lat_size), "read lat dimension");
  ThrowNetCdfError(nc_inq_dimlen(file_id, lon_dim_id, &lon_size), "read lon dimension");

  std::vector<double> lat_values(lat_size), lon_values(lon_size), values(lat_size * lon_size);

  int lat_var_id = -1, lon_var_id = -1;
  ThrowNetCdfError(nc_inq_varid(file_id, "lat", &lat_var_id), "find lat variable");
  ThrowNetCdfError(nc_inq_varid(file_id, "lon", &lon_var_id), "find lon variable");
  ThrowNetCdfError(nc_get_var_double(file_id, lat_var_id, lat_values.data()), "read lat values");
  ThrowNetCdfError(nc_get_var_double(file_id, lon_var_id, lon_values.data()), "read lon values");

  if (lat_size < 2 || lon_size < 2)
    throw std::runtime_error("SkySight NetCDF grid is too small");

  const bool lat_ascending = lat_values.front() < lat_values.back();
  const bool lon_ascending = lon_values.front() < lon_values.back();
  const double lat_step = std::abs((lat_values.back() - lat_values.front()) /
                                   double(lat_size - 1));
  const double lon_step = std::abs((lon_values.back() - lon_values.front()) /
                                   double(lon_size - 1));
  const double lat_north_edge = (lat_ascending ? lat_values.back() : lat_values.front()) +
    lat_step / 2;
  const double lon_west_edge = (lon_ascending ? lon_values.front() : lon_values.back()) -
    lon_step / 2;

  int data_var_id = -1;
  ThrowNetCdfError(nc_inq_varid(file_id, std::string{variable_name}.c_str(), &data_var_id),
                   "find data variable");
  ThrowNetCdfError(nc_get_var_double(file_id, data_var_id, values.data()),
                   "read data values");

  const double fill_value = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                       "_FillValue",
                                                       std::numeric_limits<double>::quiet_NaN());
  const double offset = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                   "add_offset", 0.0);
  const double scale = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                  "scale_factor", 1.0);

  TIFFSetErrorHandler(TiffErrorHandler);
  TIFFSetWarningHandler(TiffErrorHandler);

  TIFF *tf = XTIFFOpen(prepared.display_path.c_str(), "w");
  if (tf == nullptr)
    throw std::runtime_error("SkySight GeoTIFF open failed");

  AtScopeExit(tf) { TIFFClose(tf); };

  GTIF *gt = GTIFNew(tf);
  if (gt == nullptr)
    throw std::runtime_error("SkySight GeoTIFF metadata init failed");

  AtScopeExit(gt) { GTIFFree(gt); };

  const double tie_points[6] = {0, 0, 0, lon_west_edge, lat_north_edge, 0};
  const double pixel_scale[3] = {lon_step, lat_step, 0};
  constexpr uint16_t samples_per_pixel = 4;
  constexpr uint16_t bits_per_sample = 8;
  constexpr uint16_t alpha_sample = EXTRASAMPLE_ASSOCALPHA;

  TIFFSetField(tf, TIFFTAG_IMAGEWIDTH, lon_size);
  TIFFSetField(tf, TIFFTAG_IMAGELENGTH, lat_size);
  TIFFSetField(tf, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
  TIFFSetField(tf, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField(tf, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tf, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(tf, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tf, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(tf, TIFFTAG_EXTRASAMPLES, 1, &alpha_sample);
  TIFFSetField(tf, TIFFTAG_GEOTIEPOINTS, 6, tie_points);
  TIFFSetField(tf, TIFFTAG_GEOPIXELSCALE, 3, pixel_scale);
  TIFFSetField(tf, TIFFTAG_ROWSPERSTRIP,
               TIFFDefaultStripSize(tf, samples_per_pixel * lon_size));

  GTIFKeySet(gt, GTModelTypeGeoKey, TYPE_SHORT, 1, ModelTypeGeographic);
  GTIFKeySet(gt, GTRasterTypeGeoKey, TYPE_SHORT, 1, RasterPixelIsArea);
  GTIFKeySet(gt, GeographicTypeGeoKey, TYPE_SHORT, 1, GCS_WGS_84);
  GTIFKeySet(gt, GTCitationGeoKey, TYPE_ASCII, 25,
             "Generated by XCSoar");
  GTIFKeySet(gt, GeogLinearUnitsGeoKey, TYPE_SHORT, 1, Linear_Meter);
  GTIFKeySet(gt, GeogAngularUnitsGeoKey, TYPE_SHORT, 1, Angular_Degree);

  std::vector<uint8_t> row(samples_per_pixel * lon_size);

  for (size_t y = 0; y < lat_size; ++y) {
    std::fill(row.begin(), row.end(), 0);

    const auto source_y = lat_ascending ? (lat_size - 1 - y) : y;

    for (size_t x = 0; x < lon_size; ++x) {
      const auto source_x = lon_ascending ? x : (lon_size - 1 - x);
      const auto index = source_y * lon_size + source_x;
      const auto raw = values[index];
      if (!std::isnan(fill_value) && raw == fill_value)
        continue;

      const auto point = raw * scale + offset;
      auto color = legend.upper_bound((float)point);
      if (color == legend.begin())
        continue;

      --color;

      const auto offset_index = x * samples_per_pixel;
      row[offset_index] = color->second.red;
      row[offset_index + 1] = color->second.green;
      row[offset_index + 2] = color->second.blue;
      row[offset_index + 3] = 255;
    }

    if (TIFFWriteScanline(tf, row.data(), (uint32_t)y, 0) != 1) {
      File::Delete(prepared.display_path);
      throw std::runtime_error("SkySight GeoTIFF write failed");
    }
  }

  GTIFWriteKeys(gt);
  return CopyPath(prepared.display_path);
}

#endif

static AllocatedPath
DecodePreparedData(const SkySightPreparedData &prepared,
                   std::string_view variable_name,
                   const std::map<float, SkySight::LegendColor> &legend)
{
  if (!prepared.NeedsDecode())
    return CopyPath(prepared.display_path);

#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)
  return DecodeNetCdf(prepared, variable_name, legend);
#else
  (void)prepared;
  (void)variable_name;
  (void)legend;
  throw std::runtime_error("SkySight NetCDF decode support is unavailable in this build");
#endif
}

} // namespace

SkySightFileDecodeJob::SkySightFileDecodeJob() noexcept
  :StandbyThread("SkySightFileDecoder"),
   notify([this]{ OnNotification(); })
{
}

SkySightFileDecodeJob::~SkySightFileDecodeJob() noexcept
{
  Cancel();
}

void
SkySightFileDecodeJob::Start(SkySightPreparedData new_prepared,
                             std::string new_variable_name,
                             std::map<float, SkySight::LegendColor> new_legend,
                             std::function<void(AllocatedPath)> new_on_success,
                             std::function<void(std::exception_ptr)> new_on_error)
{
  notify.ClearNotification();

  std::unique_lock lock{mutex};
  WaitDone(lock);

  prepared = std::move(new_prepared);
  variable_name = std::move(new_variable_name);
  legend = std::move(new_legend);
  result_path = nullptr;
  error = nullptr;
  on_success = std::move(new_on_success);
  on_error = std::move(new_on_error);
  status = Status::Idle;

  Trigger();
}

void
SkySightFileDecodeJob::Cancel() noexcept
{
  notify.ClearNotification();
  LockStop();

  const std::lock_guard lock{mutex};
  result_path = nullptr;
  error = nullptr;
  on_success = {};
  on_error = {};
  status = Status::Idle;
}

SkySightFileDecodeJob::Status
SkySightFileDecodeJob::GetStatus() noexcept
{
  const std::lock_guard lock{mutex};
  return status;
}

void
SkySightFileDecodeJob::Tick() noexcept
{
  status = Status::Busy;

  auto prepared_copy = SkySightPreparedData{
    prepared.kind,
    CopyPath(prepared.source_path),
    CopyPath(prepared.display_path),
  };
  auto variable_name_copy = variable_name;
  auto legend_copy = legend;

  mutex.unlock();

  std::exception_ptr local_error;
  AllocatedPath local_result;

  try {
    local_result = DecodePreparedData(prepared_copy, variable_name_copy,
                                      legend_copy);
  } catch (...) {
    local_error = std::current_exception();
  }

  mutex.lock();

  error = std::move(local_error);
  result_path = std::move(local_result);
  status = error ? Status::Error : Status::Complete;
  notify.SendNotification();
}

void
SkySightFileDecodeJob::OnNotification() noexcept
{
  std::function<void(AllocatedPath)> success;
  std::function<void(std::exception_ptr)> failure;
  std::exception_ptr callback_error;
  AllocatedPath callback_result;

  {
    const std::lock_guard lock{mutex};
    success = on_success;
    failure = on_error;
    callback_error = error;
    if (callback_error == nullptr && result_path != nullptr)
      callback_result = CopyPath(result_path);

    result_path = nullptr;
    error = nullptr;
    on_success = {};
    on_error = {};
    status = Status::Idle;
  }

  if (callback_error != nullptr) {
    if (failure)
      failure(std::move(callback_error));
  } else if (success) {
    success(std::move(callback_result));
  }
}

SkySightPreparedData
SkySightFileDecoder::Prepare(Path path)
{
  auto payload = PrepareForecastPayload(path);

  if (payload.type == ForecastPayloadType::NetCdf)
    return PrepareNetCdfPayload(std::move(payload));

  if (IsDisplayReadyType(payload.type))
    return MakeDisplayReadyData(payload.source_path);

  throw std::runtime_error("Unsupported SkySight forecast payload");
}

void
SkySightFileDecoder::InvalidateCache(Path path) noexcept
{
  DeleteIfExists(path);

  if (path.EndsWithIgnoreCase(".zip")) {
    DeleteArchiveExtractionVariants(path);
    return;
  }

  DeletePreparedPayloadArtifacts(path);
}

bool
SkySightFileDecoder::IsNetCdfDecodeAvailable() noexcept
{
#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)
  return true;
#else
  return false;
#endif
}