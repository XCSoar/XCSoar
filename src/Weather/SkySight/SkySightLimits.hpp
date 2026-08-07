// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace SkySight {

inline constexpr std::size_t MAX_JSON_RESPONSE_BYTES = 4 * 1024 * 1024;
inline constexpr std::size_t MAX_TILE_DOWNLOAD_BYTES = 8 * 1024 * 1024;
inline constexpr std::size_t MAX_FORECAST_DOWNLOAD_BYTES = 128 * 1024 * 1024;
inline constexpr std::size_t MAX_EXPANDED_FORECAST_BYTES = 256 * 1024 * 1024;
inline constexpr std::size_t MAX_FORECAST_ARCHIVE_ENTRIES = 1024;
inline constexpr std::size_t MAX_ID_LENGTH = 63;

inline constexpr std::size_t MAX_NETCDF_GRID_AXIS = 8192;
inline constexpr std::size_t MAX_NETCDF_GRID_CELLS = 8 * 1024 * 1024;

class ResourceLimitError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

/** API identifiers are also used as components of local cache filenames. */
[[nodiscard]] constexpr bool
IsSafeId(std::string_view value) noexcept
{
  if (value.empty() || value.size() > MAX_ID_LENGTH)
    return false;

  for (const unsigned char ch : value)
    if (!((ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
      return false;

  return true;
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
