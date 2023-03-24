// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Job/Async.hpp"

#include <memory>

class OperationEnvironment;
class FileCache;
class Path;
class RasterTerrain;

/**
 * This class loads a terrain file in a separate thread.
 */
class AsyncTerrainOverviewLoader final {
  class LoaderJob;

  std::unique_ptr<LoaderJob> job;

  AsyncJobRunner async;

public:
  AsyncTerrainOverviewLoader() noexcept;
  ~AsyncTerrainOverviewLoader() noexcept;

  void Start(FileCache *cache, Path path, OperationEnvironment &env,
             UI::Notify &notify) noexcept;

  /**
   * Throws on error.
   */
  std::unique_ptr<RasterTerrain> Wait();
};
