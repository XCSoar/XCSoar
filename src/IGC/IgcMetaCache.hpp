// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "time/BrokenTime.hpp"
#include "thread/Mutex.hxx"
#include "Job/Async.hpp"
#include "Operation/Operation.hpp"

#include <deque>
#include <vector>
#include <memory>

namespace UI { class Notify; }

class IgcMetaCache {
  class FillJob;

  struct Meta {
    bool has_start{false};
    bool has_end{false};
    BrokenTime start;
    BrokenTime end;
  };

  struct CacheEntry {
    AllocatedPath path;
    Meta meta;
    StaticString<64> text;
  };

  mutable Mutex cache_mutex;
  std::deque<CacheEntry> cache;
  AsyncJobRunner async;
  QuietOperationEnvironment operation;
  std::unique_ptr<FillJob> fill_job;

  CacheEntry ParseEntry(Path path, OperationEnvironment &env);
  CacheEntry *Find(Path path) noexcept;
  void Insert(CacheEntry entry);

public:
  IgcMetaCache();
  ~IgcMetaCache() noexcept;

  /**
   * Look up compact metadata without allocating or opening the file.
   * Returns nullptr while no cache entry exists.  A non-null pointer
   * remains valid for the lifetime of the cache because deque elements
   * are never relocated or removed.
   */
  const char *GetCompactInfoPtr(Path path) noexcept;

  void StartBackgroundFill(std::vector<AllocatedPath> paths,
                           UI::Notify *notify = nullptr);
  void CancelBackgroundFill() noexcept;

  /**
   * Cancel and join the background worker.
   */
  void Shutdown() noexcept;

  /**
   * Reap a completed background fill after its UI notification.
   */
  void PollBackgroundFill() noexcept;
};
