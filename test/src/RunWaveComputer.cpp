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

#include "DebugReplay.hpp"
#include "OS/Args.hpp"
#include "Computer/WaveComputer.hpp"
#include "Computer/WaveResult.hpp"
#include "Computer/WaveSettings.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Util/Macros.hpp"

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  WaveSettings settings;
  settings.SetDefaults();
  settings.enabled = true;

  WaveComputer wave;
  wave.Reset();

  WaveResult result;
  result.Clear();

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();

    wave.Compute(basic, calculated.flight, result, settings);
  }

  delete replay;

  for (const auto &w : result.waves) {
    TCHAR time_buffer[32];
    if (w.time >= 0)
      FormatTime(time_buffer, w.time);
    else
      _tcscpy(time_buffer, _T("?"));

    _tprintf(_T("wave: t=%s location=%f,%f a=%f,%f b=%f,%f location=%s normal=%f\n"),
             time_buffer,
             (double)w.location.longitude.Degrees(),
             (double)w.location.latitude.Degrees(),
             (double)w.a.longitude.Degrees(),
             (double)w.a.latitude.Degrees(),
             (double)w.b.longitude.Degrees(),
             (double)w.b.latitude.Degrees(),
             FormatGeoPoint(w.location, CoordinateFormat::DDMMSS).c_str(),
             (double)w.normal.Degrees());
  }

  return EXIT_SUCCESS;
}
