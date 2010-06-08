/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Marks.hpp"
#include "Compatibility/string.h"
#include "LocalPath.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "resource.h"
#include "IO/TextWriter.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Projection.hpp"
#include "DateTime.hpp"

Marks::Marks()
{
  LogStartUp(TEXT("Initialise marks"));
  icon.load(IDB_MARK);
  Reset();
}

void
Marks::Reset()
{
  Poco::ScopedRWLock protect(lock, true);

  marker_store.clear();
}

Marks::~Marks()
{
  Poco::ScopedRWLock protect(lock, true);

  LogStartUp(TEXT("CloseMarks"));
  marker_store.clear();
}

void
Marks::MarkLocation(const GEOPOINT &loc,
                    const BrokenDateTime time,
                    bool play_sound)
{
  Poco::ScopedRWLock protect(lock, true);

  if (play_sound)
    PlayResource(TEXT("IDR_WAV_CLEAR"));

  marker_store.push_back(loc);

  char message[160];
  sprintf(message, "%02u.%02u.%04u\t%02u:%02u:%02u\tLon:%f\tLat:%f",
          time.day, time.month, time.year,
          time.hour, time.minute, time.second,
          (double)(loc.Longitude.value_degrees()), 
          (double)(loc.Latitude.value_degrees()));

  TCHAR fname[MAX_PATH];
  LocalPath(fname, TEXT("xcsoar-marks.txt"));
  TextWriter writer(fname, true);
  if (!writer.error())
    writer.writeln(message);
}

void Marks::Draw(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                 const Projection &projection)
{
  Poco::ScopedRWLock protect(lock, false); // read only

  if (projection.GetMapScaleUser() > fixed(30.0))
    return;

  for (unsigned i = 0; i < marker_store.size(); i++) {
    POINT sc;
    if (projection.LonLat2ScreenIfVisible(marker_store[i], &sc))
      icon.draw(canvas, bitmap_canvas, sc.x, sc.y);
  }
}
