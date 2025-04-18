// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AsyncLoader.hpp"
#include "RasterTerrain.hpp"
#include "Job/Job.hpp"
#include "system/Path.hpp"

class AsyncTerrainOverviewLoader::LoaderJob final : public Job {
  FileCache *const cache;
  const AllocatedPath path;
  std::unique_ptr<RasterTerrain> terrain;

public:
  LoaderJob(FileCache *_cache, Path _path) noexcept
    :cache(_cache), path(_path) {}

  std::unique_ptr<RasterTerrain> &&Finish() noexcept {
    return std::move(terrain);
  }

  void Run(OperationEnvironment &env) override {
    terrain = RasterTerrain::OpenTerrain(cache, path, env);
  }
};

AsyncTerrainOverviewLoader::AsyncTerrainOverviewLoader() noexcept = default;

AsyncTerrainOverviewLoader::~AsyncTerrainOverviewLoader() noexcept
{
  if (async.IsBusy()) {
    async.Cancel();
    try {
      async.Wait();
    } catch (...) {
    }
  }
}

void
AsyncTerrainOverviewLoader::Start(FileCache *cache, Path path,
                                  OperationEnvironment &env,
                                  UI::Notify &notify) noexcept
{
  job = std::make_unique<LoaderJob>(cache, path);
  async.Start(job.get(), env, &notify);
}

std::unique_ptr<RasterTerrain>
AsyncTerrainOverviewLoader::Wait()
{
  assert(job);

  async.Wait();
  return job->Finish();
}
