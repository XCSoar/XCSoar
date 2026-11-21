// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class FileCache;
class Path;
class OperationEnvironment;
class TerrainBase;

/**
 * Open elevation from a map zip: \c terrain.tif if present, else a
 * COG cache built from \c terrain.jp2.  Topography stays in the zip.
 */
std::unique_ptr<TerrainBase>
OpenXcmTerrain(FileCache *cache, Path map_path,
               OperationEnvironment &operation);
