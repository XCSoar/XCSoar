/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Replay/IgcReplayGlue.hpp"
#include "Replay/NmeaReplayGlue.hpp"
#include "Replay/DemoReplayGlue.hpp"

#include <tchar.h>
#include <windef.h> /* for MAX_PATH */
#include <stdio.h>

class ProtectedTaskManager;

class Replay
{
public:
  Replay(ProtectedTaskManager& task_manager):
    mode(MODE_NULL),
    Demo(task_manager) {}

  bool Update();
  void Stop();
  void Start();
  const TCHAR* GetFilename();
  void SetFilename(const TCHAR *name);

  fixed GetTimeScale();
  void SetTimeScale(const fixed TimeScale);

private:
  enum ReplayMode {
    MODE_NULL,
    MODE_IGC,
    MODE_NMEA,
    MODE_DEMO
  };

  ReplayMode mode;
  IgcReplayGlue Igc;
  NmeaReplayGlue Nmea;
  DemoReplayGlue Demo;
};

#endif
