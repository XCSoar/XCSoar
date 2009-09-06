/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Marks.h"
#include "Protection.hpp"
#include "Topology.h"
#include "Dialogs.h"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Compatibility/string.h"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "Audio/Sound.hpp"
#include "LogFile.hpp"
#include "resource.h"

#include <assert.h>

#include "wcecompat/ts_string.h"
// TODO code: check ts_string does the right thing

//////////////////////////////////////////////////

Marks::Marks(const char* name):topo_marks(name, Color(0xD0,0xD0,0xD0)) {
  ScopeLock protect(*GetMutex());
  StartupStore(TEXT("Initialise marks\n"));
  topo_marks.scaleThreshold = 30.0;
  topo_marks.loadBitmap(IDB_MARK);
  Reset();
}

// TODO code: - This convert to non-unicode will not support all languages
//		(some may use more complicated PATH names, containing Unicode)
//  char buffer[MAX_PATH];
//  ConvertTToC(buffer, LocalPath(TEXT("xcsoar-marks")));
// DISABLED LocalPath
// JMW localpath does NOT work for the shapefile renderer!

void Marks::Reset() {
  ScopeLock protect(*GetMutex());
  topo_marks.Reset();
}

Marks::~Marks() {
  StartupStore(TEXT("CloseMarks\n"));
  ScopeLock protect(*GetMutex());
  topo_marks.DeleteFiles();
}

#include "Interface.hpp"

void Marks::MarkLocation(const double lon, const double lat)
{
  ScopeLock protect(*GetMutex());

#ifndef DISABLEAUDIO
  if (XCSoarInterface::SettingsComputer().EnableSoundModes) {
    PlayResource(TEXT("IDR_WAV_CLEAR"));
  }
#endif
  topo_marks.addPoint(lon, lat);
  topo_marks.triggerUpdateCache = true;

  char message[160];

  sprintf(message,"Lon:%f Lat:%f\r\n", lon, lat);

  FILE *stream;
  TCHAR fname[MAX_PATH];
  LocalPath(fname,TEXT("xcsoar-marks.txt"));
  stream = _tfopen(fname,TEXT("a+"));
  if (stream != NULL){
    fwrite(message,strlen(message),1,stream);
    fclose(stream);
  }
}


void Marks::Draw(Canvas &canvas, MapWindow &m_window, const RECT rc)
{
  ScopeLock protect(*GetMutex());
  topo_marks.Paint(canvas, m_window, rc);
}
