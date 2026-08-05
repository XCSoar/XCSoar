// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <span>
#include <string_view>

namespace SkySight {

/** Versioned NetCDF→GeoTIFF product (plain .tif is legacy). */
inline constexpr std::string_view DECODED_OVERLAY_SUFFIX = ".v2.tif";

/** TIFF path endings (provider images and legacy overlays). */
inline constexpr std::string_view TIFF_SUFFIXES[] = {
  ".tif", ".tiff",
};

/** JPEG path endings. */
inline constexpr std::string_view JPEG_SUFFIXES[] = {
  ".jpg", ".jpeg",
};

/**
 * Display-ready image suffixes on a path (includes plain .tif).
 * Used for "is this already an image?" checks.
 */
inline constexpr std::string_view DISPLAY_IMAGE_SUFFIXES[] = {
  ".tif", ".tiff", ".png", ".jpg", ".jpeg",
};

/**
 * Directory globs for selectable forecast overlays.
 * Omits plain *.tif so legacy near-zero washes are not chosen.
 */
inline constexpr std::string_view DISPLAY_IMAGE_GLOBS[] = {
  "*.v2.tif", "*.png", "*.jpg", "*.jpeg",
};

/**
 * Alternate image suffixes tried beside a cached payload path when the
 * path itself is not already a display file.
 */
inline constexpr std::string_view ALTERNATE_DISPLAY_IMAGE_SUFFIXES[] = {
  ".png", ".jpg", ".jpeg", ".tiff",
};

/** Raw download containers visited during cache cleanup. */
inline constexpr std::string_view RAW_FORECAST_GLOBS[] = {
  "*.nc", "*.min", "*.zip",
};

/** Inner type peeled after a trailing ".min" gzip wrapper. */
inline constexpr std::string_view MIN_WRAPPED_INNER_SUFFIXES[] = {
  ".nc", ".tif", ".tiff", ".png",
};

/**
 * Simple forecast artifact suffixes peeled for cache stem parsing
 * (after .v2.tif and .min handling).
 */
inline constexpr std::string_view FORECAST_ARTIFACT_SUFFIXES[] = {
  ".zip", ".nc", ".jpg", ".tif", ".tiff", ".png", ".jpeg",
};

/**
 * Path endings that identify a forecast data entry (case-sensitive).
 */
inline constexpr std::string_view FORECAST_DATA_PATH_SUFFIXES[] = {
  ".nc", ".nc.min",
  ".tif", ".tiff", ".tif.min", ".tiff.min",
  ".png", ".png.min",
  ".jpg", ".jpeg",
};

/** Overlay products removed when invalidating a payload. */
inline constexpr std::string_view DERIVED_OVERLAY_SUFFIXES[] = {
  ".v2.tif", ".tif", ".tiff", ".png", ".jpg", ".jpeg",
};

/** Extra siblings removed when invalidating a zip extract. */
inline constexpr std::string_view RAW_EXTRACT_SUFFIXES[] = {
  ".min", ".nc",
};

/** URL / path suffixes that need NetCDF decode after download. */
inline constexpr std::string_view NETCDF_DECODE_URL_SUFFIXES[] = {
  ".nc", ".nc.min", ".min",
};

[[nodiscard]] constexpr bool
EndsWithAny(std::string_view path,
            std::span<const std::string_view> suffixes) noexcept
{
  for (const auto suffix : suffixes)
    if (path.ends_with(suffix))
      return true;

  return false;
}

[[nodiscard]] constexpr bool
EqualsAny(std::string_view value,
          std::span<const std::string_view> options) noexcept
{
  for (const auto option : options)
    if (value == option)
      return true;

  return false;
}

[[nodiscard]] inline bool
PathEndsWithAnyIgnoreCase(
  Path path, std::span<const std::string_view> suffixes) noexcept
{
  for (const auto suffix : suffixes)
    if (path.EndsWithIgnoreCase(suffix.data()))
      return true;

  return false;
}

[[nodiscard]] inline bool
RemoveAnySuffix(std::string_view &stem,
                std::span<const std::string_view> suffixes) noexcept
{
  for (const auto suffix : suffixes)
    if (RemoveSuffix(stem, suffix))
      return true;

  return false;
}

[[nodiscard]] constexpr bool
HasForecastDataSuffix(std::string_view path) noexcept
{
  return EndsWithAny(path, FORECAST_DATA_PATH_SUFFIXES);
}

/**
 * Peel a known forecast artifact suffix, leaving
 * "{region}-{layer}-{YYYY-MM-DD-HHMM}".
 */
[[nodiscard]] inline std::string_view
StripForecastArtifactSuffix(std::string_view filename) noexcept
{
  auto stem = filename;

  if (RemoveSuffix(stem, DECODED_OVERLAY_SUFFIX))
    return stem;

  /* Compressed downloads end in ".nc.min", ".tif.min", … */
  if (RemoveSuffix(stem, std::string_view{".min"})) {
    if (RemoveAnySuffix(stem, MIN_WRAPPED_INNER_SUFFIXES))
      return stem;

    return {};
  }

  if (RemoveAnySuffix(stem, FORECAST_ARTIFACT_SUFFIXES))
    return stem;

  return {};
}

} // namespace SkySight
