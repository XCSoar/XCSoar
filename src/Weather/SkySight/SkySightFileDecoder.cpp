// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightFileDecoder.hpp"
#include "FieldImage.hpp"
#include "SkySightLimits.hpp"
#include "SkySightPayloadSuffixes.hpp"

#include "LogFile.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "io/ZipArchive.hpp"
#include "io/ZipReader.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/zlib/GunzipReader.hxx"
#include "system/FileUtil.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"

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
#endif

using namespace std::string_view_literals;

namespace {

using CancellationCheck = std::function<bool()>;

void
ThrowIfCancelled(const CancellationCheck &is_cancelled)
{
  if (is_cancelled && is_cancelled())
    throw std::runtime_error("SkySight forecast decode cancelled");
}

[[nodiscard]] AllocatedPath
CopyPath(Path path)
{
  return AllocatedPath(path.c_str());
}

[[nodiscard]] AllocatedPath
CopyOptionalPath(Path path)
{
  return path != nullptr ? CopyPath(path) : AllocatedPath{};
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

[[nodiscard]] bool
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

[[nodiscard]] std::span<const std::byte>
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

[[nodiscard]] ForecastPayloadType
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

  if (SkySight::PathEndsWithAnyIgnoreCase(path, SkySight::TIFF_SUFFIXES))
    return ForecastPayloadType::Tiff;

  if (path.EndsWithIgnoreCase(".png"))
    return ForecastPayloadType::Png;

  if (SkySight::PathEndsWithAnyIgnoreCase(path, SkySight::JPEG_SUFFIXES))
    return ForecastPayloadType::Jpeg;

  return ForecastPayloadType::Unknown;
}

[[nodiscard]] bool
IsDisplayReadyType(ForecastPayloadType type) noexcept
{
  return type == ForecastPayloadType::Tiff ||
    type == ForecastPayloadType::Png ||
    type == ForecastPayloadType::Jpeg;
}

[[nodiscard]] const char *
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

[[nodiscard]] AllocatedPath
ReplacePath(Path source_path, AllocatedPath target_path)
{
  if (target_path == source_path)
    return CopyPath(source_path);

  File::Delete(target_path);
  if (!File::Replace(source_path, target_path))
    throw std::runtime_error("Failed to normalise SkySight forecast payload suffix");

  return target_path;
}

/**
 * Peel a trailing ".min" gzip wrapper suffix from a mutable path string.
 */
[[nodiscard]] bool
StripMinSuffix(std::string &path) noexcept
{
  auto view = std::string_view{path};
  if (!RemoveSuffix(view, ".min"sv))
    return false;

  path.resize(view.size());
  return true;
}

[[nodiscard]] AllocatedPath
GetNormalisedPayloadTarget(Path source_path, const char *suffix)
{
  std::string path{source_path.c_str()};
  if (!StripMinSuffix(path))
    return AllocatedPath(source_path.WithSuffix(suffix).c_str());

  Path base_path{path.c_str()};
  if (base_path.EndsWithIgnoreCase(suffix))
    return AllocatedPath(base_path.c_str());

  return AllocatedPath(base_path.WithSuffix(suffix).c_str());
}

[[nodiscard]] AllocatedPath
NormalisePayloadPath(Path source_path, ForecastPayloadType type)
{
  const auto *suffix = GetPayloadSuffix(type);
  if (suffix == nullptr)
    return CopyPath(source_path);

  return ReplacePath(source_path,
                     GetNormalisedPayloadTarget(source_path, suffix));
}

void
DeleteIfExists(Path path) noexcept
{
  if (File::Exists(path))
    File::Delete(path);
}

[[nodiscard]] bool
NeedsGunzipForecastPayload(Path path) noexcept;

[[nodiscard]] AllocatedPath
GetGunzipOutputPath(Path compressed_path);

/**
 * Delete overlay products derived from @p path.  When @p include_raw_extracts
 * is true (zip invalidation), also remove `.min` / `.nc` siblings.
 */
void
DeleteDerivedArtifacts(Path path,
                       bool include_raw_extracts = false) noexcept
{
  if (include_raw_extracts)
    for (const auto suffix : SkySight::RAW_EXTRACT_SUFFIXES)
      DeleteIfExists(path.WithSuffix(suffix.data()));

  /* Prefer the versioned NetCDF overlay suffix; also remove legacy .tif
     washes from earlier decoders that painted near-zero opaque. */
  for (const auto suffix : SkySight::DERIVED_OVERLAY_SUFFIXES)
    DeleteIfExists(path.WithSuffix(suffix.data()));
}

void
DeletePreparedPayloadArtifacts(Path path) noexcept
{
  if (NeedsGunzipForecastPayload(path)) {
    const auto inflated_path = GetGunzipOutputPath(path);
    DeleteIfExists(inflated_path);
    DeleteDerivedArtifacts(inflated_path);
    DeleteDerivedArtifacts(path);
    return;
  }

  DeleteDerivedArtifacts(path);
}

void
CopyReader(Reader &reader, OutputStream &output, std::size_t maximum_size,
           const CancellationCheck &is_cancelled)
{
  const auto advertised_size = reader.GetSize();
  if (advertised_size > maximum_size)
    throw SkySight::ResourceLimitError(
      "Expanded SkySight forecast exceeds its size limit");

  std::size_t remaining = maximum_size;
  std::array<std::byte, 64 * 1024> buffer;
  while (true) {
    ThrowIfCancelled(is_cancelled);
    const auto nbytes = reader.Read(buffer);
    if (nbytes == 0)
      break;

    if (nbytes > remaining)
      throw SkySight::ResourceLimitError(
        "Expanded SkySight forecast exceeds its size limit");

    output.Write(std::span<const std::byte>{buffer.data(), nbytes});
    remaining -= nbytes;
  }
}

/**
 * Return @p output_path when it already exists and is within the size
 * limit.  When @p source_path is set, also require the output not be older
 * than the source (used after gunzip so a stale inflate is rebuilt).
 */
[[nodiscard]] AllocatedPath
ReuseExpandedPayloadIfPresent(Path output_path, Path source_path)
{
  if (!File::Exists(output_path))
    return nullptr;

  if (source_path != nullptr &&
      File::GetLastModification(output_path) <
        File::GetLastModification(source_path))
    return nullptr;

  if (File::GetSize(output_path) > SkySight::MAX_EXPANDED_FORECAST_BYTES)
    throw SkySight::ResourceLimitError(
      "Expanded SkySight forecast exceeds its size limit");

  return AllocatedPath(output_path.c_str());
}

void
WriteExpandedPayload(Reader &reader, Path output_path,
                     const CancellationCheck &is_cancelled)
{
  FileOutputStream output(output_path);
  CopyReader(reader, output, SkySight::MAX_EXPANDED_FORECAST_BYTES,
             is_cancelled);
  output.Commit();
}

[[nodiscard]] bool
NeedsGunzipForecastPayload(Path path) noexcept
{
  return path.EndsWithIgnoreCase(".min");
}

[[nodiscard]] AllocatedPath
GetGunzipOutputPath(Path compressed_path)
{
  std::string output_value{compressed_path.c_str()};
  (void)StripMinSuffix(output_value);

  if (Path{output_value.c_str()}.GetSuffix() == nullptr)
    return Path{output_value.c_str()}.WithSuffix(".nc");

  return AllocatedPath(output_value.c_str());
}

[[nodiscard]] AllocatedPath
InflateForecastPayload(Path compressed_path,
                       const CancellationCheck &is_cancelled)
{
  const auto output_path = GetGunzipOutputPath(compressed_path);
  if (auto existing = ReuseExpandedPayloadIfPresent(output_path,
                                                    compressed_path);
      existing != nullptr)
    return existing;

  FileReader file(compressed_path);
  GunzipReader gunzip(file);
  WriteExpandedPayload(gunzip, output_path, is_cancelled);
  return AllocatedPath(output_path.c_str());
}

[[nodiscard]] AllocatedPath
ExtractArchiveEntry(Path archive_path, const CancellationCheck &is_cancelled)
{
  ZipArchive archive(archive_path);

  std::string fallback_entry_name;
  std::string entry_name;
  std::size_t entry_count = 0;
  while (true) {
    ThrowIfCancelled(is_cancelled);
    entry_name = archive.NextName();
    if (entry_name.empty())
      break;

    if (++entry_count > SkySight::MAX_FORECAST_ARCHIVE_ENTRIES)
      throw SkySight::ResourceLimitError(
        "SkySight forecast archive contains too many entries");

    if (entry_name.back() == '/')
      continue;

    if (fallback_entry_name.empty())
      fallback_entry_name = entry_name;

    if (SkySight::HasForecastDataSuffix(entry_name))
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

  if (auto existing = ReuseExpandedPayloadIfPresent(output_path, nullptr);
      existing != nullptr)
    return existing;

  ZipReader reader(archive.get(), entry_name.c_str());
  WriteExpandedPayload(reader, output_path, is_cancelled);
  return AllocatedPath(output_path.c_str());
}

[[nodiscard]] PreparedForecastPayload
PrepareForecastPayload(Path path, const CancellationCheck &is_cancelled)
{
  ThrowIfCancelled(is_cancelled);
  PreparedForecastPayload payload{
    AllocatedPath(path.c_str()),
    {},
    DetectForecastPayloadType(path),
  };

  if (payload.type == ForecastPayloadType::Zip) {
    payload.source_path = ExtractArchiveEntry(payload.source_path,
                                              is_cancelled);
    payload.type = DetectForecastPayloadType(payload.source_path);
  }

  if (payload.type == ForecastPayloadType::Gzip) {
    payload.source_path = InflateForecastPayload(payload.source_path,
                                                 is_cancelled);
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

[[nodiscard]] SkySightPreparedData
MakeDisplayReadyData(Path path)
{
  return {
    SkySightPreparedDataKind::DisplayReady,
    CopyPath(path),
    CopyPath(path),
  };
}

[[nodiscard]] SkySightPreparedData
PrepareNetCdfPayload(PreparedForecastPayload payload)
{
  auto display_path = payload.source_path.WithSuffix(
    SkySight::DECODED_OVERLAY_SUFFIX.data());
  auto cleanup_source_path = CopyPath(payload.source_path);

  /* Drop overlays written by earlier decoders, which are neither
     displayed nor rebuilt, so they do not linger in the cache. */
  DeleteIfExists(payload.source_path.WithSuffix(".tif"));
  for (const auto suffix : SkySight::LEGACY_DECODED_OVERLAY_SUFFIXES)
    DeleteIfExists(payload.source_path.WithSuffix(suffix.data()));

  if (File::Exists(display_path) &&
      File::GetLastModification(display_path) >=
        File::GetLastModification(payload.source_path)) {
    DeleteIfExists(payload.source_path);
    if (payload.cleanup_download_path != nullptr)
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

[[nodiscard]] SkySightPreparedData
PreparePayload(Path path, const CancellationCheck &is_cancelled)
{
  if (File::GetSize(path) > SkySight::MAX_FORECAST_DOWNLOAD_BYTES)
    throw SkySight::ResourceLimitError(
      "SkySight forecast exceeds its size limit");

  auto payload = PrepareForecastPayload(path, is_cancelled);
  ThrowIfCancelled(is_cancelled);

  if (payload.type == ForecastPayloadType::NetCdf)
    return PrepareNetCdfPayload(std::move(payload));

  if (IsDisplayReadyType(payload.type))
    return MakeDisplayReadyData(payload.source_path);

  throw std::runtime_error("Unsupported SkySight forecast payload");
}

#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)

void
ThrowNetCdfError(int status, const char *action);

void
ValidateCoordinateVariable(int file_id, int variable_id,
                           int expected_dimension, size_t expected_size,
                           const char *name)
{
  int dimensions = 0;
  ThrowNetCdfError(nc_inq_varndims(file_id, variable_id, &dimensions), name);
  if (dimensions != 1)
    throw FmtRuntimeError("SkySight NetCDF {} variable is not one-dimensional",
                          name);

  int dimension_id = -1;
  ThrowNetCdfError(nc_inq_vardimid(file_id, variable_id, &dimension_id), name);
  size_t size = 0;
  ThrowNetCdfError(nc_inq_dimlen(file_id, dimension_id, &size), name);
  if (dimension_id != expected_dimension || size != expected_size)
    throw FmtRuntimeError("SkySight NetCDF {} dimension does not match its grid",
                          name);
}

void
ValidateDataVariable(int file_id, int variable_id,
                     int latitude_dimension, int longitude_dimension)
{
  int dimensions = 0;
  ThrowNetCdfError(nc_inq_varndims(file_id, variable_id, &dimensions),
                   "inspect data dimensions");
  if (dimensions != 2)
    throw std::runtime_error("SkySight NetCDF data variable is not two-dimensional");

  int dimension_ids[2];
  ThrowNetCdfError(nc_inq_vardimid(file_id, variable_id, dimension_ids),
                   "inspect data dimensions");
  if (dimension_ids[0] != latitude_dimension ||
      dimension_ids[1] != longitude_dimension)
    throw std::runtime_error("SkySight NetCDF data dimensions do not match the grid");
}

void
ThrowNetCdfError(int status, const char *action)
{
  if (status != NC_NOERR)
    throw FmtRuntimeError("SkySight NetCDF {}: {}",
                          action, nc_strerror(status));
}

double
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

AllocatedPath
DecodeNetCdf(const SkySightPreparedData &prepared,
             std::string_view variable_name,
             const CancellationCheck &is_cancelled)
{
  ThrowIfCancelled(is_cancelled);

  const std::string temporary_name =
    std::string{prepared.display_path.c_str()} + ".tmp";
  const AllocatedPath temporary_path{temporary_name.c_str()};
  DeleteIfExists(temporary_path);
  AtScopeExit(&temporary_path) { DeleteIfExists(temporary_path); };

  int file_id = -1;
  ThrowNetCdfError(nc_open(prepared.source_path.c_str(), NC_NOWRITE, &file_id),
                   "open");
  AtScopeExit(file_id) { if (file_id >= 0) nc_close(file_id); };
  ThrowIfCancelled(is_cancelled);

  int lat_dim_id = -1, lon_dim_id = -1;
  ThrowNetCdfError(nc_inq_dimid(file_id, "lat", &lat_dim_id), "find lat dimension");
  ThrowNetCdfError(nc_inq_dimid(file_id, "lon", &lon_dim_id), "find lon dimension");

  size_t lat_size = 0, lon_size = 0;
  ThrowNetCdfError(nc_inq_dimlen(file_id, lat_dim_id, &lat_size), "read lat dimension");
  ThrowNetCdfError(nc_inq_dimlen(file_id, lon_dim_id, &lon_size), "read lon dimension");

  if (!SkySight::IsNetCdfGridSizeAllowed(lat_size, lon_size))
    throw SkySight::ResourceLimitError(
      "SkySight NetCDF grid exceeds its size limit");

  int lat_var_id = -1, lon_var_id = -1;
  ThrowNetCdfError(nc_inq_varid(file_id, "lat", &lat_var_id), "find lat variable");
  ThrowNetCdfError(nc_inq_varid(file_id, "lon", &lon_var_id), "find lon variable");
  ValidateCoordinateVariable(file_id, lat_var_id, lat_dim_id, lat_size, "lat");
  ValidateCoordinateVariable(file_id, lon_var_id, lon_dim_id, lon_size, "lon");

  std::vector<double> lat_values(lat_size), lon_values(lon_size);
  ThrowNetCdfError(nc_get_var_double(file_id, lat_var_id, lat_values.data()), "read lat values");
  ThrowNetCdfError(nc_get_var_double(file_id, lon_var_id, lon_values.data()), "read lon values");
  ThrowIfCancelled(is_cancelled);

  if (std::any_of(lat_values.begin(), lat_values.end(),
                  [](double value) { return !std::isfinite(value); }) ||
      std::any_of(lon_values.begin(), lon_values.end(),
                  [](double value) { return !std::isfinite(value); }))
    throw std::runtime_error("SkySight NetCDF coordinates are not finite");

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
  if (!std::isfinite(lat_step) || !std::isfinite(lon_step) ||
      lat_step <= 0 || lon_step <= 0 ||
      !std::isfinite(lat_north_edge) || !std::isfinite(lon_west_edge))
    throw std::runtime_error("SkySight NetCDF coordinates are invalid");

  int data_var_id = -1;
  ThrowNetCdfError(nc_inq_varid(file_id, std::string{variable_name}.c_str(), &data_var_id),
                   "find data variable");
  ValidateDataVariable(file_id, data_var_id, lat_dim_id, lon_dim_id);

  std::vector<double> values(lat_size * lon_size);
  const size_t start[2] = {0, 0};
  const size_t count[2] = {lat_size, lon_size};
  ThrowNetCdfError(nc_get_vara_double(file_id, data_var_id, start, count,
                                     values.data()),
                   "read data values");
  ThrowIfCancelled(is_cancelled);

  const double fill_value = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                       "_FillValue",
                                                       std::numeric_limits<double>::quiet_NaN());
  const double offset = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                   "add_offset", 0.0);
  const double scale = GetOptionalDoubleAttribute(file_id, data_var_id,
                                                  "scale_factor", 1.0);
  if (!std::isfinite(offset) || !std::isfinite(scale))
    throw std::runtime_error("SkySight NetCDF scaling is not finite");

  /* Move the samples into display orientation (north-west first) and
     apply the packing attributes.  What gets stored is the field itself,
     not a picture of it: the map contours the patch it is showing, which
     a whole-region image could never resolve finely enough. */
  SkySight::ScalarField field;
  field.width = unsigned(lon_size);
  field.height = unsigned(lat_size);
  field.values.resize(lat_size * lon_size);

  for (size_t y = 0; y < lat_size; ++y) {
    const auto source_y = lat_ascending ? (lat_size - 1 - y) : y;

    for (size_t x = 0; x < lon_size; ++x) {
      const auto source_x = lon_ascending ? x : (lon_size - 1 - x);
      const auto raw = values[source_y * lon_size + source_x];

      auto sample = std::numeric_limits<float>::quiet_NaN();
      if (std::isfinite(raw) &&
          (std::isnan(fill_value) || raw != fill_value)) {
        const auto point = raw * scale + offset;
        if (std::isfinite(point) && std::isfinite((float)point))
          sample = (float)point;
      }

      field.values[y * lon_size + x] = sample;
    }
  }

  values.clear();
  values.shrink_to_fit();
  ThrowIfCancelled(is_cancelled);

  SkySight::GeoScalarField geo_field;
  geo_field.field = std::move(field);
  geo_field.north_west = GeoPoint{Angle::Degrees(lon_west_edge),
                                  Angle::Degrees(lat_north_edge)};
  geo_field.longitude_step = lon_step;
  geo_field.latitude_step = lat_step;

  SkySight::WriteFieldImage(temporary_path, geo_field);

  ThrowIfCancelled(is_cancelled);
  if (!File::Replace(temporary_path, prepared.display_path))
    throw std::runtime_error("SkySight forecast field publication failed");

  DeleteIfExists(prepared.cleanup_source_path);
  if (prepared.cleanup_download_path != nullptr)
    DeleteIfExists(prepared.cleanup_download_path);
  return CopyPath(prepared.display_path);
}

#endif

AllocatedPath
DecodePreparedData(const SkySightPreparedData &prepared,
                   std::string_view variable_name,
                   const CancellationCheck &is_cancelled)
{
  ThrowIfCancelled(is_cancelled);
  auto prepared_payload = prepared.kind ==
    SkySightPreparedDataKind::NeedsPreparation
    ? PreparePayload(prepared.source_path, is_cancelled)
    : SkySightPreparedData{
        prepared.kind,
        CopyOptionalPath(prepared.source_path),
        CopyOptionalPath(prepared.display_path),
        CopyOptionalPath(prepared.cleanup_source_path),
        CopyOptionalPath(prepared.cleanup_download_path),
      };

  ThrowIfCancelled(is_cancelled);
  if (!prepared_payload.NeedsDecode())
    return CopyPath(prepared_payload.display_path);

#if defined(USE_GEOTIFF) && defined(HAVE_SKYSIGHT_NETCDF)
  return DecodeNetCdf(prepared_payload, variable_name, is_cancelled);
#else
  (void)prepared_payload;
  (void)variable_name;
  (void)is_cancelled;
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
                             std::function<void(AllocatedPath)> new_on_success,
                             std::function<void(std::exception_ptr)> new_on_error)
{
  notify.ClearNotification();

  std::unique_lock lock{mutex};
  WaitDone(lock);
  cancel_requested.store(false, std::memory_order_relaxed);

  prepared = std::move(new_prepared);
  variable_name = std::move(new_variable_name);
  result_path = nullptr;
  error = nullptr;
  on_success = std::move(new_on_success);
  on_error = std::move(new_on_error);
  status = Status::Busy;

  Trigger();
}

void
SkySightFileDecodeJob::Cancel() noexcept
{
  cancel_requested.store(true, std::memory_order_relaxed);
  notify.ClearNotification();
  LockStop();
  notify.ClearNotification();

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
    CopyOptionalPath(prepared.source_path),
    CopyOptionalPath(prepared.display_path),
    CopyOptionalPath(prepared.cleanup_source_path),
    CopyOptionalPath(prepared.cleanup_download_path),
  };
  auto variable_name_copy = variable_name;

  mutex.unlock();

  std::exception_ptr local_error;
  AllocatedPath local_result;

  try {
    local_result = DecodePreparedData(prepared_copy, variable_name_copy,
                                      [this] {
                                        return cancel_requested.load(
                                          std::memory_order_relaxed);
                                      });
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
SkySightFileDecoder::MakeDeferredPreparation(Path path) noexcept
{
  return {
    SkySightPreparedDataKind::NeedsPreparation,
    CopyPath(path),
  };
}

namespace {

[[nodiscard]] AllocatedPath
FindExistingPath(Path path) noexcept
{
  return File::Exists(path) ? CopyPath(path) : nullptr;
}

/**
 * Locate a decoded NetCDF overlay.  Only the current versioned product is
 * accepted, so images written by an earlier decoder are re-rendered
 * instead of reused.
 */
[[nodiscard]] AllocatedPath
FindNetCdfOverlay(Path path) noexcept
{
  return FindExistingPath(
    path.WithSuffix(SkySight::DECODED_OVERLAY_SUFFIX.data()));
}

[[nodiscard]] AllocatedPath
FindProviderImage(Path path) noexcept
{
  if (SkySight::PathEndsWithAnyIgnoreCase(
        path, SkySight::DISPLAY_IMAGE_SUFFIXES) &&
      File::Exists(path))
    return CopyPath(path);

  for (const auto suffix : SkySight::ALTERNATE_DISPLAY_IMAGE_SUFFIXES) {
    const auto candidate = path.WithSuffix(suffix.data());
    if (File::Exists(candidate))
      return AllocatedPath(candidate.c_str());
  }

  return nullptr;
}

} // namespace

AllocatedPath
SkySightFileDecoder::FindCachedDisplay(Path path)
{
  if (auto display = FindProviderImage(path); display != nullptr)
    return display;

  if (auto display = FindNetCdfOverlay(path); display != nullptr)
    return display;

  if (NeedsGunzipForecastPayload(path)) {
    const auto inflated = GetGunzipOutputPath(path);
    if (auto display = FindNetCdfOverlay(inflated); display != nullptr)
      return display;
  }

  return nullptr;
}

void
SkySightFileDecoder::InvalidateCache(Path path) noexcept
{
  DeleteIfExists(path);

  if (path.EndsWithIgnoreCase(".zip")) {
    DeleteDerivedArtifacts(path, true);
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
