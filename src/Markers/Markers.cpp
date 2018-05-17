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

#include "Markers.hpp"
#include "Geo/GeoPoint.hpp"
#include "Time/BrokenDateTime.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "LogFile.hpp"
#include "LocalPath.hpp"

#include <stdexcept>

void
MarkLocation(const GeoPoint &loc, const BrokenDateTime &time)
try {
  assert(time.IsPlausible());

  FileOutputStream file(LocalPath(_T("xcsoar-marks.txt")),
                        FileOutputStream::Mode::APPEND_OR_CREATE);
  BufferedOutputStream os(file);
  os.Format("%02u.%02u.%04u\t%02u:%02u:%02u\tLon:%f\tLat:%f\n",
            time.day, time.month, time.year,
            time.hour, time.minute, time.second,
            (double)loc.longitude.Degrees(),
            (double)loc.latitude.Degrees());
  os.Flush();
  file.Commit();
} catch (const std::runtime_error &e) {
  LogError(e);
}
