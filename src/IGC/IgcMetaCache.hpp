// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "thread/Mutex.hxx"
#include "Job/Async.hpp"
#include "Operation/Operation.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace UI { class DelayedNotify; class Notify; }

/**
 * One scanned IGC file. @c detected is set only when both takeoff and
 * landing were found. @c duration is then the time between those two.
 */
struct IgcCachedFlight {
  std::chrono::seconds duration{};
  bool detected{false};
};

class IgcMetaCache {
  class FillJob;

  struct CacheEntry {
    AllocatedPath path;
    StaticString<64> text;
    std::chrono::seconds duration{};
    bool detected{false};
  };

  mutable Mutex cache_mutex;
  std::deque<CacheEntry> cache;
  AsyncJobRunner async;
  QuietOperationEnvironment operation;
  std::unique_ptr<FillJob> fill_job;

  CacheEntry ParseEntry(Path path, OperationEnvironment &env);
  CacheEntry *FindUnlocked(Path path) noexcept;
  CacheEntry *Find(Path path) noexcept;
  void Insert(CacheEntry entry);
  void JoinFill() noexcept;

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

  /** Empty until this file has been scanned. */
  std::optional<IgcCachedFlight> GetFlight(Path path) noexcept;

  void StartBackgroundFill(std::vector<AllocatedPath> paths,
                           UI::DelayedNotify *progress_notify,
                           UI::Notify *completion_notify);
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
