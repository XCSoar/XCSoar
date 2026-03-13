// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"
#include "time/BrokenDateTime.hpp"

#include <vector>

class AllocatedPath;
class ProgressListener;
class CurlGlobal;

namespace EDL {

struct CachedDay {
  BrokenDateTime day;
  unsigned file_count;

  [[gnu::pure]]
  bool IsComplete() const noexcept;
};

Co::Task<AllocatedPath>
EnsureDownloaded(BrokenDateTime forecast, unsigned isobar,
                 CurlGlobal &curl, ProgressListener &progress);

Co::Task<unsigned>
EnsureDayDownloaded(BrokenDateTime day, CurlGlobal &curl,
                    ProgressListener &progress);

std::vector<CachedDay>
ListDownloadedDays() noexcept;

unsigned
DeleteOtherDownloadedDays(BrokenDateTime keep_day) noexcept;

} // namespace EDL
