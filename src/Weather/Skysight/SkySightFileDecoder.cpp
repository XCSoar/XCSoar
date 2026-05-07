// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightFileDecoder.hpp"

#include "io/FileOutputStream.hxx"
#include "io/ZipArchive.hpp"
#include "io/ZipReader.hpp"
#include "system/FileUtil.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

[[nodiscard]] static bool
HasForecastDataSuffix(std::string_view path) noexcept
{
  return path.ends_with(".nc") ||
    path.ends_with(".tif") || path.ends_with(".tiff") ||
    path.ends_with(".png") ||
    path.ends_with(".jpg") || path.ends_with(".jpeg");
}

[[nodiscard]] static bool
IsDisplayReadySuffix(Path path) noexcept
{
  return path.EndsWithIgnoreCase(".tif") ||
    path.EndsWithIgnoreCase(".tiff") ||
    path.EndsWithIgnoreCase(".png") ||
    path.EndsWithIgnoreCase(".jpg") ||
    path.EndsWithIgnoreCase(".jpeg");
}

[[nodiscard]] static bool
NeedsNetCdfDecode(Path path) noexcept
{
  return path.EndsWithIgnoreCase(".nc");
}

[[nodiscard]] static AllocatedPath
ExtractArchiveEntry(Path archive_path)
{
  ZipArchive archive(archive_path);

  std::string entry_name;
  while (true) {
    entry_name = archive.NextName();
    if (entry_name.empty())
      throw std::runtime_error("SkySight forecast archive is empty");

    if (entry_name.back() != '/' && HasForecastDataSuffix(entry_name))
      break;
  }

  const auto suffix = Path{entry_name.c_str()}.GetSuffix();
  if (suffix == nullptr)
    throw std::runtime_error("SkySight forecast archive entry has no suffix");

  const auto output_path = archive_path.WithSuffix(suffix);
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

} // namespace

SkySightPreparedData
SkySightFileDecoder::Prepare(Path path)
{
  AllocatedPath source_path = path.EndsWithIgnoreCase(".zip")
    ? ExtractArchiveEntry(path)
    : AllocatedPath(path.c_str());

  if (IsDisplayReadySuffix(source_path))
    return {
      SkySightPreparedDataKind::DisplayReady,
      AllocatedPath(source_path.c_str()),
      AllocatedPath(source_path.c_str()),
    };

  if (NeedsNetCdfDecode(source_path))
    return {
      SkySightPreparedDataKind::NeedsNetCdfDecode,
      AllocatedPath(source_path.c_str()),
      source_path.WithSuffix(".tif"),
    };

  throw std::runtime_error("Unsupported SkySight forecast payload");
}