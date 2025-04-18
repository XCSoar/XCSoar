// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/tstring.hpp"

#include <list>

#include <tchar.h>

struct PCMetSettings;
class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

namespace PCMet {

struct OverlayInfo {
  enum class Type {
    VERTICAL,
    COUNT
  };

  enum class Area {
    GERMANY_NORTH,
    GERMANY_SOUTH,
    COUNT
  };

  Type type;
  Area area;
  unsigned level;
  unsigned step;

  tstring label;
  AllocatedPath path;
};

[[gnu::pure]]
std::list<OverlayInfo> CollectOverlays();

struct Overlay {
  BrokenDateTime run_time, valid_time;
  AllocatedPath path;

  Overlay(BrokenDateTime _run_time, BrokenDateTime _valid_time, Path _path)
    :run_time(_run_time), valid_time(_valid_time), path(_path) {}

  Overlay(BrokenDateTime _run_time, BrokenDateTime _valid_time,
          AllocatedPath &&_path)
    :run_time(_run_time), valid_time(_valid_time), path(std::move(_path)) {}

  bool IsDefined() const {
    return path != nullptr;
  }
};

Co::Task<Overlay>
DownloadOverlay(const OverlayInfo &info, BrokenDateTime now_utc,
                const PCMetSettings &settings,
                CurlGlobal &curl, ProgressListener &progress);

} // namespace PCMet
