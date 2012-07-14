/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "OS/PathName.hpp"

#include <assert.h>

void
Replay::Stop()
{
  if (replay != NULL)
    replay->Stop();
}

void
Replay::Start()
{
  if (replay == NULL)
    replay = &demo_replay;

  replay->Start();
}

const TCHAR*
Replay::GetFilename()
{
  return path;
}

void
Replay::SetFilename(const TCHAR *name)
{
  assert(name != NULL);

  _tcscpy(path, name);

  if (StringIsEmpty(name)) {
    replay = &demo_replay;
    return;
  }

  Stop();

  if (MatchesExtension(name, _T(".igc"))) {
    igc_replay.SetFilename(name);
    replay = &igc_replay;
  } else {
    nmea_replay.SetFilename(name);
    replay = &nmea_replay;
  }
}


bool
Replay::Update()
{
  if (replay != NULL)
    replay->Update(time_scale);

  return false;
}
