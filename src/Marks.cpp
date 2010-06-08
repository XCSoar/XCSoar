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
#include "SettingsComputer.hpp"
#include "Compatibility/string.h"
#include "LocalPath.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "resource.h"
#include "IO/TextWriter.hpp"
#include "Navigation/GeoPoint.hpp"

#include <assert.h>

Marks::Marks(const char* name, const SETTINGS_COMPUTER &_settings_computer) :
  topo_marks(name, Color(0xD0, 0xD0, 0xD0)),
  settings_computer(_settings_computer)
{
  LogStartUp(TEXT("Initialise marks"));
  topo_marks.scaleThreshold = 30.0;
  topo_marks.loadIcon(IDB_MARK);
  Reset();
}

void
Marks::Reset()
{
  Poco::ScopedRWLock protect(lock, true);
  topo_marks.Reset();
}

Marks::~Marks()
{
  LogStartUp(TEXT("CloseMarks"));
  Poco::ScopedRWLock protect(lock, true);
  topo_marks.DeleteFiles();
}

void
Marks::MarkLocation(const GEOPOINT &loc)
{
  Poco::ScopedRWLock protect(lock, true);

  if (settings_computer.EnableSoundModes)
    PlayResource(TEXT("IDR_WAV_CLEAR"));

  topo_marks.addPoint(loc);
  topo_marks.triggerUpdateCache = true;

  char message[160];

  sprintf(message, "Lon:%f Lat:%f",
          (double)(loc.Longitude.value_degrees()), 
          (double)(loc.Latitude.value_degrees()));

  TCHAR fname[MAX_PATH];
  LocalPath(fname, TEXT("xcsoar-marks.txt"));
  TextWriter writer(fname, true);
  if (!writer.error())
    writer.writeln(message);
}

void Marks::Draw(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                 const Projection &projection, LabelBlock &label_block,
                 const SETTINGS_MAP &settings_map)
{
  Poco::ScopedRWLock protect(lock, false); // read only
  topo_marks.Paint(canvas, bitmap_canvas, projection,
                   label_block, settings_map);
}
