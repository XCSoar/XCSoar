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

#include "Markers.hpp"
#include "IO/TextWriter.hpp"
#include "IO/DataFile.hpp"

void
Markers::Reset()
{
  marker_store.clear();
}

void
Markers::MarkLocation(const GeoPoint &loc, const BrokenDateTime &time)
{
  assert(time.IsPlausible());

  Marker marker = { loc, time };
  marker_store.push_back(marker);

  char message[160];
  sprintf(message, "%02u.%02u.%04u\t%02u:%02u:%02u\tLon:%f\tLat:%f",
          time.day, time.month, time.year,
          time.hour, time.minute, time.second,
          (double)(loc.longitude.Degrees()), 
          (double)(loc.latitude.Degrees()));

  TextWriter *writer = CreateDataTextFile(_T("xcsoar-marks.txt"), true);
  if (writer != NULL) {
    writer->WriteLine(message);
    delete writer;
  }
}
