// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterLocation.hpp"
#include "Height.hpp"
#include "Geo/GeoBounds.hpp"
#include "thread/SharedMutex.hpp"

#include <memory>
#include <vector>
#include <cstdint>
#include <string>

struct zzip_dir;
class Path;
class RasterTileCache;
class OperationEnvironment;
class CurlGlobal;

/**
 * Information about a single pyramid level in a COG file.
 */
struct CogLevel {
  unsigned width;
  unsigned height;
  unsigned tile_width;
  unsigned tile_height;
  unsigned tiles_x;
  unsigned tiles_y;

  /**
   * Resolution relative to full resolution (level 0 = 1, level 1 = 2, etc.)
   */
  unsigned scale;

  /**
   * File offsets for each tile in this level (for HTTP access).
   * Index = ty * tiles_x + tx
   */
  std::vector<uint64_t> tile_offsets;
  std::vector<uint64_t> tile_byte_counts;
};

/**
 * Loader for Cloud-Optimized GeoTIFF (COG) terrain files.
 *
 * COG files provide efficient random access to tiled elevation data
 * with built-in pyramid levels (overviews), enabling fast rendering
 * at any zoom level without scanning the entire file.
 *
 * Supports two backends:
 * - Local files: Uses libtiff for direct file access
 * - HTTP URLs: Uses HTTP byte-range requests (CoGetRange)
 */
class CogLoader {
public:
  static constexpr unsigned MAX_LEVELS = 16;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;

public:
  CogLoader() noexcept;
  ~CogLoader() noexcept;

  CogLoader(const CogLoader &) = delete;
  CogLoader &operator=(const CogLoader &) = delete;

  /**
   * Check if a path/URL appears to be a TIFF/COG file by extension.
   */
  [[gnu::pure]]
  static bool IsCogFile(const char *path) noexcept;

  /**
   * Check if a string is an HTTP/HTTPS URL.
   */
  [[gnu::pure]]
  static bool IsHttpUrl(const char *path) noexcept;

  /**
   * Open a COG file from a local filesystem path.
   * Uses libtiff for file access.
   *
   * @return true on success
   */
  bool Open(Path path);

  /**
   * Open a COG file from a zip archive.
   *
   * @param dir the zip archive containing the terrain file
   * @param path path to the COG file within the archive
   * @return true on success
   */
  bool Open(struct zzip_dir *dir, const char *path);

  /**
   * Open a COG file from an HTTP URL.
   * Uses byte-range requests to fetch header and tiles on demand.
   *
   * @param curl the curl instance for HTTP requests
   * @param url the HTTP/HTTPS URL of the COG file
   * @return true on success
   */
  bool OpenHttp(CurlGlobal &curl, const char *url);

  void Close() noexcept;

  [[gnu::pure]]
  bool IsOpen() const noexcept;

  /**
   * @return true if this loader is using HTTP backend
   */
  [[gnu::pure]]
  bool IsHttp() const noexcept;

  /**
   * Get the geographic bounds of the terrain.
   */
  [[gnu::pure]]
  const GeoBounds &GetBounds() const noexcept;

  /**
   * Get the full resolution size in pixels.
   */
  [[gnu::pure]]
  UnsignedPoint2D GetSize() const noexcept;

  /**
   * Get the number of pyramid levels.
   */
  [[gnu::pure]]
  unsigned GetLevelCount() const noexcept;

  /**
   * Get information about a specific pyramid level.
   */
  [[gnu::pure]]
  const CogLevel &GetLevel(unsigned level) const noexcept;

  /**
   * Select the best pyramid level for a given meters-per-pixel scale.
   */
  [[gnu::pure]]
  unsigned SelectLevel(double meters_per_pixel) const noexcept;

  /**
   * Read a single tile from the COG file.
   *
   * @param level pyramid level (0 = full resolution)
   * @param tx tile column index
   * @param ty tile row index
   * @param buffer output buffer for elevation data (must be tile_width * tile_height)
   * @return true on success
   */
  bool ReadTile(unsigned level, unsigned tx, unsigned ty,
                TerrainHeight *buffer) noexcept;

  /**
   * Get the tile size for a given level.
   */
  [[gnu::pure]]
  UnsignedPoint2D GetTileSize(unsigned level) const noexcept;

  /**
   * Get the URL (for HTTP mode) or empty string (for local mode).
   */
  [[gnu::pure]]
  const std::string &GetUrl() const noexcept;

  /**
   * Get terrain height at a geographic location.
   * Uses appropriate pyramid level based on quantisation parameter.
   * Returns TerrainHeight::Invalid() if location is outside bounds.
   *
   * @param location geographic location to query
   * @param quantisation desired pixel quantisation (1=full resolution,
   *        higher values select lower-resolution pyramid levels)
   */
  [[gnu::pure]]
  TerrainHeight GetHeight(const GeoPoint &location,
                          unsigned quantisation = 1) const noexcept;
};

/**
 * Check if a terrain file is a COG (by extension).
 */
[[gnu::pure]]
bool
IsCogTerrainFile(const char *filename) noexcept;

/**
 * Load terrain from a COG file into a RasterTileCache.
 *
 * This function loads the overview and tile metadata from the COG,
 * but does not load full-resolution tiles (those are demand-loaded).
 *
 * @param dir the zip archive, or nullptr for filesystem access
 * @param path path to the COG file
 * @param cache the tile cache to populate
 * @param env operation environment for progress reporting
 */
void
LoadCogTerrain(struct zzip_dir *dir, const char *path,
               RasterTileCache &cache,
               OperationEnvironment &env);

/**
 * Load terrain from a COG file (filesystem path).
 */
void
LoadCogTerrain(Path path, RasterTileCache &cache,
               OperationEnvironment &env);

/**
 * Update terrain tiles from a COG file based on current view.
 *
 * @param dir the zip archive, or nullptr
 * @param path path to the COG file
 * @param cache the tile cache
 * @param mutex mutex protecting the cache
 * @param center current view center in raster coordinates
 * @param radius view radius in pixels
 */
void
UpdateCogTiles(struct zzip_dir *dir, const char *path,
               RasterTileCache &cache, SharedMutex &mutex,
               SignedRasterLocation center, unsigned radius);
