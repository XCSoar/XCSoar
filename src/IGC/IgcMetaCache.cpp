// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IgcMetaCache.hpp"

#include "IGC/IGCParser.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Job/Async.hpp"
#include "Job/Job.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
#include "io/FileLineReader.hpp"
#include "LogFile.hpp"

#include <chrono>
#include <cstring>
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

static bool
ParseBRecordTime(const char *line, BrokenTime &time,
                 bool &gps_valid) noexcept
{
  if (line[0] != 'B' || std::strlen(line) < 25 ||
      !IGCParseTime(line + 1, time))
    return false;

  if (line[24] == 'A')
    gps_valid = true;
  else if (line[24] == 'V')
    gps_valid = false;
  else
    return false;

  return true;
}

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

  try {
    FileLineReaderA reader(path);
    unsigned line_count = 0;
    char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      if ((++line_count % 256) == 0 && env.IsCancelled())
        throw OperationCancelled{};

      BrokenTime time;
      bool gps_valid;
      if (ParseBRecordTime(line, time, gps_valid) && gps_valid) {
        if (!entry.meta.has_start) {
          entry.meta.start = time;
          entry.meta.has_start = true;
        }
        entry.meta.end = time;
        entry.meta.has_end = true;
      }
    }

    if (env.IsCancelled())
      throw OperationCancelled{};
  } catch (const OperationCancelled &) {
    throw;
  } catch (...) {
    LogError(std::current_exception(), "Failed to read IGC metadata");
  }

  entry.text = "";

  if (entry.meta.has_start && entry.meta.has_end) {
    StaticString<32> lbuf;
    lbuf.Format("%02u:%02u - %02u:%02u",
                (unsigned)entry.meta.start.hour,
                (unsigned)entry.meta.start.minute,
                (unsigned)entry.meta.end.hour,
                (unsigned)entry.meta.end.minute);
    entry.text = lbuf.c_str();

    int64_t s = (int64_t)entry.meta.start.GetSecondOfDay();
    int64_t e = (int64_t)entry.meta.end.GetSecondOfDay();
    int64_t diff = e - s;
    if (diff < 0)
      diff += 24 * 3600;
    auto dur = FormatTimespanSmart(std::chrono::seconds(diff), 2);
    entry.text.append(" (");
    entry.text.append(dur.c_str());
    entry.text.append(")");
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
