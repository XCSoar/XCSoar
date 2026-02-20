// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "time/BrokenTime.hpp"
#include "co/InjectTask.hxx"
#include "thread/Mutex.hxx"

#include <atomic>
#include <deque>
#include <string>
#include <vector>
#include <memory>

namespace UI { class Notify; }

class IgcMetaCache {
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
  std::unique_ptr<Co::InjectTask> inject_task;
  std::atomic<UI::Notify *> current_notify{nullptr};

  CacheEntry ParseEntry(Path path) noexcept;
  Co::InvokeTask FillCacheCoro(std::vector<AllocatedPath> paths) noexcept;
  void OnFillComplete(std::exception_ptr error) noexcept;
  CacheEntry *FindOrParse(Path path) noexcept;

public:
  ~IgcMetaCache() noexcept;

  /**
   * Get compact metadata for an IGC file: "HH:MM - HH:MM (duration)".
   *
   * Returns a safe copy of the cached metadata. The first call for a given
   * path parses the file; subsequent calls return the cached result.
   */
  std::string GetCompactInfo(Path path) noexcept;

  void StartBackgroundFill(std::vector<AllocatedPath> paths,
                           UI::Notify *notify = nullptr) noexcept;
  void CancelBackgroundFill() noexcept;

  /**
   * Non-blocking. Returns immediately; completion is signalled via
   * `OnFillComplete()` and the `UI::Notify` passed to
   * `StartBackgroundFill()`.
   */
  void PollBackgroundFill() noexcept;
};
