/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef REPLAY_HPP
#define REPLAY_HPP

#include "Event/Timer.hpp"
#include "Math/fixed.hpp"
#include "NMEA/Info.hpp"
#include "Time/PeriodClock.hpp"

#include <tchar.h>
#include <windef.h> /* for MAX_PATH */

class Logger;
class ProtectedTaskManager;
class AbstractReplay;
class CatmullRomInterpolator;

class Replay final
  : private Timer
{
  fixed time_scale;

  AbstractReplay *replay;

  Logger *logger;
  ProtectedTaskManager &task_manager;

  TCHAR path[MAX_PATH];

  /**
   * The time of day according to replay input.  This is negative if
   * unknown.
   */
  fixed virtual_time;

  /**
   * Keeps track of the wall-clock time between two Update() calls.
   */
  PeriodClock clock;

  /**
   * The last NMEAInfo returned by the #AbstractReplay instance.  It
   * is held back until #virtual_time has passed #next_data.time.
   */
  NMEAInfo next_data;

  CatmullRomInterpolator *cli;

public:
  Replay(Logger *_logger, ProtectedTaskManager &_task_manager)
    :time_scale(fixed(1)), replay(nullptr),
     logger(_logger), task_manager(_task_manager), cli(nullptr) {
    path[0] = _T('\0');
  }

  ~Replay() {
    Stop();
  }

  bool IsActive() const {
    return replay != nullptr;
  }

private:
  bool Update();

public:
  void Stop();
  bool Start(const TCHAR *_path);

  const TCHAR *GetFilename() const {
    return path;
  }

  fixed GetTimeScale() const {
    return time_scale;
  }

  void SetTimeScale(const fixed _time_scale) {
    time_scale = _time_scale;
  }

private:
  virtual void OnTimer() override;
};

#endif
