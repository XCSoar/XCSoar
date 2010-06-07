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

#include "Topology/TopologyWriter.hpp"

#include "wcecompat/ts_string.h"
#include "Screen/Util.hpp"
#include "Projection.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "SettingsUser.hpp"
#include "Navigation/GeoPoint.hpp"

#include <stdlib.h>
#include <tchar.h>

#include "LogFile.hpp"

TopologyWriter::~TopologyWriter()
{
  if (!shapefileopen)
    return;

  Close();
  DeleteFiles();
}

TopologyWriter::TopologyWriter(const char* shpname, const Color thecolor) :
  Topology(shpname, thecolor, true)
{
  Reset();
}

void
TopologyWriter::DeleteFiles(void)
{
  // Delete all files, since zziplib interface doesn't handle file modes
  // properly
  if (strlen(filename) <= 0)
    return;

  TCHAR fname[MAX_PATH];

  ascii2unicode(filename, fname);
  _tcscat(fname, TEXT(".shp"));
  DeleteFile(fname);

  ascii2unicode(filename, fname);
  _tcscat(fname, TEXT(".shx"));
  DeleteFile(fname);

  ascii2unicode(filename, fname);
  _tcscat(fname, TEXT(".dbf"));
  DeleteFile(fname);
}

void
TopologyWriter::CreateFiles(void)
{
  // by default, now, this overwrites previous contents
  if (msSHPCreateFile(&shpfile, filename, SHP_POINT) == -1)
    return;

  char dbfname[100];
  strcpy(dbfname, filename);
  strcat(dbfname, ".dbf");
  shpfile.hDBF = msDBFCreate(dbfname);

  shapefileopen = true;
  Close();
}

void
TopologyWriter::Reset(void)
{
  if (shapefileopen)
    Close();

  DeleteFiles();
  CreateFiles();

  Open();
}

void
TopologyWriter::addPoint(const GEOPOINT &gp)
{
  pointObj p = { gp.Longitude.value_degrees(), 
                 gp.Latitude.value_degrees(), 0.0 };

  if (shapefileopen) {
    msSHPWritePoint(shpfile.hSHP, &p);
    Close();
  }

  Open();
}
