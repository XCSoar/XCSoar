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

#include "Replay.hpp"
#include "IgcReplayGlue.hpp"
#include "NmeaReplayGlue.hpp"
#include "DemoReplayGlue.hpp"
#include "Util/StringUtil.hpp"
#include "OS/PathName.hpp"

#include <assert.h>

void
Replay::Stop()
{
  if (replay) {
    replay->Stop();
    replay.reset();
  }
}

void
Replay::Start(const TCHAR *_path)
{
  assert(_path != NULL);

  /* make sure the old AbstractReplay instance has cleaned up before
     creating a new one */
  replay.reset();

  _tcscpy(path, path);

  if (StringIsEmpty(path)) {
    replay.reset(new DemoReplayGlue(task_manager));
  } else if (MatchesExtension(path, _T(".igc"))) {
    auto r = new IgcReplayGlue(logger);
    r->SetFilename(path);
    replay.reset(r);
  } else {
    auto r = new NmeaReplayGlue();
    r->SetFilename(path);
    replay.reset(r);
  }

  replay->Start();
}

bool
Replay::Update()
{
  if (replay)
    replay->Update(time_scale);

  return false;
}
