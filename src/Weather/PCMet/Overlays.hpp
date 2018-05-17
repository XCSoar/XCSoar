/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_PCMET_OVERLAYS_HPP
#define XCSOAR_PCMET_OVERLAYS_HPP

#include "OS/Path.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Util/tstring.hpp"

#include <list>

#include <tchar.h>

struct PCMetSettings;
class JobRunner;

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

  OverlayInfo()
    :path(nullptr) {}
};

gcc_pure
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
    return !path.IsNull();
  }
};

Overlay
DownloadOverlay(const OverlayInfo &info, BrokenDateTime now_utc,
                const PCMetSettings &settings,
                JobRunner &runner);

};

#endif
