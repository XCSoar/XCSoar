// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainBase.hpp"
#include "CogFromXcm.hpp"
#include "RasterTraits.hpp"
#include "Profile/Profile.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"

#include <stdexcept>

void
TerrainBase::Load([[maybe_unused]] OperationEnvironment &operation)
{
  const UnsignedPoint2D size = raster.GetSize();
  if (size.x == 0 || size.y == 0)
    throw std::runtime_error("COG terrain has zero size");

  const GeoBounds &bounds = raster.GetBounds();
  if (!bounds.IsValid())
    throw std::runtime_error("COG terrain has no valid geographic bounds");

  if (raster.GetLevelCount() == 0)
    throw std::runtime_error("COG has no pyramid levels");

  projection.Set(bounds,
                 {size.x << RasterTraits::SUBPIXEL_BITS,
                  size.y << RasterTraits::SUBPIXEL_BITS});
}

std::unique_ptr<TerrainBase>
TerrainBase::OpenCog(Path path, OperationEnvironment &operation)
{
  auto terrain = std::make_unique<TerrainBase>();

  if (!terrain->raster.Open(path))
    throw std::runtime_error("Failed to open COG terrain file");

  terrain->Load(operation);
  return terrain;
}

bool
TerrainBase::UpdateTiles([[maybe_unused]] const GeoPoint &location,
                         [[maybe_unused]] double radius) noexcept
{
  if (raster.ProcessScanTile() || raster.ProcessOneInternalTile())
    ++serial;
  return raster.HasPendingInternalTiles();
}

std::unique_ptr<TerrainBase>
TerrainBase::OpenTerrain(FileCache *cache, Path path,
                         OperationEnvironment &operation)
{
  if (CogRaster::IsCogFile(path.c_str())) {
    LogFormat("Opening COG terrain file: %s", path.c_str());
    return OpenCog(path, operation);
  }

  return OpenXcmTerrain(cache, path, operation);
}

std::unique_ptr<TerrainBase>
TerrainBase::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
try {
  const auto terrain_path = Profile::GetPath(ProfileKeys::TerrainFile);
  if (terrain_path != nullptr &&
      CogRaster::IsCogFile(terrain_path.c_str())) {
    LogFormat("Opening terrain: %s", terrain_path.c_str());
    return OpenTerrain(cache, terrain_path, operation);
  }

  const auto map_path = Profile::GetPath(ProfileKeys::MapFile);
  if (map_path != nullptr)
    return OpenXcmTerrain(cache, map_path, operation);

  /* Map database is the zip; some profiles still have the xcm in
     TerrainFile with MapFile empty. */
  if (terrain_path != nullptr) {
    LogFormat("Opening terrain from map: %s", terrain_path.c_str());
    return OpenXcmTerrain(cache, terrain_path, operation);
  }

  return nullptr;
} catch (...) {
  LogError(std::current_exception(), "Failed to open terrain");
  operation.SetError(std::current_exception());
  throw;
}
