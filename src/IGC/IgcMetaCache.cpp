// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IgcMetaCache.hpp"

#include "IGC/FlightTimes.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Job/Async.hpp"
#include "Job/Job.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
#include "LogFile.hpp"
#include <utility>

class IgcMetaCache::FillJob final : public Job {
  IgcMetaCache &cache;
  std::vector<AllocatedPath> paths;

public:
  FillJob(IgcMetaCache &_cache, std::vector<AllocatedPath> &&_paths) noexcept
    :cache(_cache), paths(std::move(_paths)) {}

  void Run(OperationEnvironment &env) override {
    for (const auto &path : paths) {
      if (env.IsCancelled())
        break;

      if (cache.Find(Path(path.c_str())) == nullptr)
        cache.Insert(cache.ParseEntry(Path(path.c_str()), env));
    }
  }
};

IgcMetaCache::IgcMetaCache() = default;

IgcMetaCache::~IgcMetaCache() noexcept
{
  Shutdown();
}

IgcMetaCache::CacheEntry
IgcMetaCache::ParseEntry(Path path, OperationEnvironment &env)
{
  CacheEntry entry;
  entry.path = path;
  entry.text = "";

  try {
    const auto times = DetectIGCFlightTimes(path, &env);
    entry.meta.has_start = times.has_valid_fixes;
    entry.meta.has_end = times.has_valid_fixes;
    if (times.has_valid_fixes)
      entry.meta.start = times.takeoff;
    if (times.has_valid_fixes)
      entry.meta.end = times.landing;

    if (entry.meta.has_start && entry.meta.has_end) {
      StaticString<32> lbuf;
      lbuf.Format("%02u:%02u - %02u:%02u",
                  (unsigned)entry.meta.start.hour,
                  (unsigned)entry.meta.start.minute,
                  (unsigned)entry.meta.end.hour,
                  (unsigned)entry.meta.end.minute);
      entry.text = lbuf.c_str();

      auto dur = FormatTimespanSmart(times.duration, 2);
      entry.text.append(" (");
      entry.text.append(dur.c_str());
      entry.text.append(")");
    }
  } catch (const OperationCancelled &) {
    throw;
  } catch (...) {
    LogError(std::current_exception(), "Failed to read IGC metadata");
  }

  return entry;
}

IgcMetaCache::CacheEntry *
IgcMetaCache::Find(Path path) noexcept
{
  const std::lock_guard lock{cache_mutex};
  for (auto &e : cache) {
    if (e.path == path)
      return &e;
  }

  return nullptr;
}

void
IgcMetaCache::Insert(CacheEntry entry)
{
  const std::lock_guard lock{cache_mutex};
  for (const auto &e : cache)
    if (e.path == entry.path)
      return;

  cache.push_back(std::move(entry));
}

const char *
IgcMetaCache::GetCompactInfoPtr(Path path) noexcept
{
  CacheEntry *entry = Find(path);
  return entry != nullptr ? entry->text.c_str() : nullptr;
}

void
IgcMetaCache::StartBackgroundFill(std::vector<AllocatedPath> paths,
                                  UI::Notify *notify)
{
  if (async.IsBusy())
    CancelBackgroundFill();

  fill_job = std::make_unique<FillJob>(*this, std::move(paths));
  try {
    async.Start(fill_job.get(), operation, notify);
  } catch (...) {
    fill_job.reset();
    throw;
  }
}

void
IgcMetaCache::CancelBackgroundFill() noexcept
{
  if (!async.IsBusy())
    return;

  async.Cancel();
  try {
    async.Wait();
  } catch (const OperationCancelled &) {
  } catch (...) {
    LogError(std::current_exception(), "IGC metadata worker failed");
  }

  fill_job.reset();
}

void
IgcMetaCache::Shutdown() noexcept
{
  CancelBackgroundFill();
}

void
IgcMetaCache::PollBackgroundFill() noexcept
{
  if (!async.IsBusy() || !async.HasFinished())
    return;

  try {
    async.Wait();
  } catch (const OperationCancelled &) {
  } catch (...) {
    LogError(std::current_exception(), "IGC metadata worker failed");
  }

  fill_job.reset();
}
