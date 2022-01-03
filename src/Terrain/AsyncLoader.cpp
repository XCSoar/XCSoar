/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
