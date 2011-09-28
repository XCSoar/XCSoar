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

#include "Replay/Replay.hpp"

#include "Util/StringUtil.hpp"
#include "UtilsFile.hpp"

void
Replay::Stop()
{
  switch (mode) {
  case MODE_IGC:
    igc_replay.Stop();
    break;
  case MODE_NMEA:
    nmea_replay.Stop();
    break;
  case MODE_DEMO:
    demo_replay.Stop();
    mode = MODE_NULL;
    break;
  case MODE_NULL:
    break;
  };
}

void
Replay::Start()
{
  switch (mode) {
  case MODE_IGC:
    igc_replay.Start();
    break;
  case MODE_NMEA:
    nmea_replay.Start();
    break;
  case MODE_DEMO:
    demo_replay.Start();
    break;
  case MODE_NULL:
    demo_replay.Start();
    mode = MODE_DEMO;
    break;
  };
}

const TCHAR*
Replay::GetFilename()
{
  return (mode== MODE_IGC ? igc_replay.GetFilename() : nmea_replay.GetFilename());
}

void
Replay::SetFilename(const TCHAR *name)
{
  if (!name || string_is_empty(name)) {
    mode = MODE_DEMO;
    return;
  }

  Stop();

  if (MatchesExtension(name, _T(".igc"))) {
    mode = MODE_IGC;
    igc_replay.SetFilename(name);
  } else {
    mode = MODE_NMEA;
    nmea_replay.SetFilename(name);
  }
}


bool
Replay::Update()
{
  switch (mode) {
  case MODE_IGC:
    return igc_replay.Update();
  case MODE_NMEA:
    return nmea_replay.Update();
  case MODE_DEMO:
    return demo_replay.Update();
  case MODE_NULL:
    break;
  };
  return false;
}

fixed
Replay::GetTimeScale()
{
  switch (mode) {
  case MODE_IGC:
    return igc_replay.time_scale;
  case MODE_NMEA:
    return nmea_replay.time_scale;
  case MODE_DEMO:
    return demo_replay.time_scale;
  case MODE_NULL:
    break;
  };
  return fixed_one;
}

void
Replay::SetTimeScale(const fixed time_scale)
{
  igc_replay.time_scale = time_scale;
  nmea_replay.time_scale = time_scale;
  demo_replay.time_scale = time_scale;
}
