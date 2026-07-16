// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <ctime>
#include <stdexcept>
#include <string_view>

namespace SkySight {

inline constexpr std::size_t MAX_JSON_RESPONSE_BYTES = 4 * 1024 * 1024;
inline constexpr std::size_t MAX_TILE_DOWNLOAD_BYTES = 8 * 1024 * 1024;
inline constexpr std::size_t MAX_FORECAST_DOWNLOAD_BYTES = 128 * 1024 * 1024;
inline constexpr std::size_t MAX_EXPANDED_FORECAST_BYTES = 256 * 1024 * 1024;
inline constexpr std::size_t MAX_FORECAST_ARCHIVE_ENTRIES = 1024;

inline constexpr std::size_t MAX_NETCDF_GRID_AXIS = 8192;
inline constexpr std::size_t MAX_NETCDF_GRID_CELLS = 8 * 1024 * 1024;

inline constexpr unsigned MAX_TILE_FAILURES = 3;
inline constexpr std::size_t MAX_TILE_FAILURES_PER_GENERATION = 256;

/** Maximum accepted length for region/layer ids used in cache filenames. */
inline constexpr std::size_t MAX_ID_LENGTH = 64;

/**
 * Reject path separators and other unsafe characters before interpolating
 * provider ids into cache filenames.
 */
[[nodiscard]] constexpr bool
IsSafeId(std::string_view id) noexcept
{
  if (id.empty() || id.size() > MAX_ID_LENGTH)
    return false;

  if (id == "." || id == "..")
    return false;

  for (const char ch : id) {
    const bool ok =
      (ch >= 'A' && ch <= 'Z') ||
      (ch >= 'a' && ch <= 'z') ||
      (ch >= '0' && ch <= '9') ||
      ch == '_' || ch == '-' || ch == '.';
    if (!ok)
      return false;
  }

  return true;
}

class ResourceLimitError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

[[nodiscard]] constexpr time_t
GetTileRetryDelay(unsigned failures) noexcept
{
  return failures <= 1 ? 10 : failures == 2 ? 30 : 60;
}

[[nodiscard]] constexpr bool
ShouldSuppressTile(unsigned failures) noexcept
{
  return failures >= MAX_TILE_FAILURES;
}

[[nodiscard]] constexpr bool
IsNetCdfGridSizeAllowed(std::size_t latitude_size,
                        std::size_t longitude_size) noexcept
{
  return latitude_size >= 2 && longitude_size >= 2 &&
    latitude_size <= MAX_NETCDF_GRID_AXIS &&
    longitude_size <= MAX_NETCDF_GRID_AXIS &&
    latitude_size <= MAX_NETCDF_GRID_CELLS / longitude_size;
}

} // namespace SkySight
