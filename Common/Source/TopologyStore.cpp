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

#include "TopologyStore.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "MapWindow.h"
#include "Topology.h"
#include "Dialogs.h"
#include "Language.hpp"
#include "Compatibility/string.h"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "SettingsUser.hpp" // for EnableTopology
#include <assert.h>

#include "wcecompat/ts_string.h"
// TODO code: check ts_string does the right thing

//////////////////////////////////////////////////

void TopologyStore::TriggerUpdateCaches() {
  ScopeLock protect(*GetMutex());
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      topology_store[z]->triggerUpdateCache=true;
    }
  }
  if (topo_marks) {
    topo_marks->triggerUpdateCache = true;
  }
}


void TopologyStore::ScanVisibility(MapWindowProjection &m_projection,
				   MapWindowTimer &m_timer,
				   rectObj &_bounds_active,
				   const bool force) {
  ScopeLock protect(*GetMutex());

  // check if things have come into or out of scale limit
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      topology_store[z]->TriggerIfScaleNowVisible(m_projection);
    }
  }
  // ok, now update the caches

  if (topo_marks) {
    topo_marks->updateCache(m_projection, _bounds_active);
  } // projection, bounds

  if (EnableTopology) {
    // check if any needs to have cache updates because wasnt
    // visible previously when bounds moved
    bool sneaked= false;
    bool rta;

    // we will make sure we update at least one cache per call
    // to make sure eventually everything gets refreshed

    int total_shapes_visible = 0;
    for (int z=0; z<MAXTOPOLOGY; z++) {
      if (topology_store[z]) {
	rta = m_timer.RenderTimeAvailable() || force || !sneaked;
	if (topology_store[z]->triggerUpdateCache) {
	  sneaked = true;
	}
	topology_store[z]->updateCache(m_projection, _bounds_active, !rta);
	total_shapes_visible += topology_store[z]->getNumVisible();
      }
    }
#ifdef DEBUG_GRAPHICS
    DebugStore("%d # shapes\n", total_shapes_visible);
#endif
  }
}


void TopologyStore::Close() 
{
  StartupStore(TEXT("CloseTopology\n"));
  ScopeLock protect(*GetMutex());
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      delete topology_store[z];
    }
  }
}


void TopologyStore::Draw(Canvas &canvas, MapWindow &m_window, const RECT rc)
{
  ScopeLock protect(*GetMutex());
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      topology_store[z]->Paint(canvas,m_window,rc);
    }
  }
}


void TopologyStore::Open() {
  StartupStore(TEXT("OpenTopology\n"));
  CreateProgressDialog(gettext(TEXT("Loading Topology File...")));
  ScopeLock protect(*GetMutex());

  // Start off by getting the names and paths
  static TCHAR  szOrigFile[MAX_PATH] = TEXT("\0");
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");
  static  TCHAR Directory[MAX_PATH] = TEXT("\0");

  for (int z=0; z<MAXTOPOLOGY; z++) {
    topology_store[z] = 0;
  }

  GetRegistryString(szRegistryTopologyFile, szFile, MAX_PATH);
  ExpandLocalPath(szFile);
  _tcscpy(szOrigFile,szFile); // make copy of original
  ContractLocalPath(szOrigFile);

  // remove it in case it causes a crash (will restore later)
  SetRegistryString(szRegistryTopologyFile, TEXT("\0"));

  if (_tcslen(szFile)==0) {

    // file is blank, so look for it in a map file
    static TCHAR  szMapFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    if (_tcslen(szMapFile)==0) {
      return;
    }
    ExpandLocalPath(szMapFile);

    // Look for the file within the map zip file...
    _tcscpy(Directory,szMapFile);
    _tcscat(Directory,TEXT("/"));
    szFile[0]=0;
    _tcscat(szFile,Directory);
    _tcscat(szFile,TEXT("topology.tpl"));

  } else {
    ExtractDirectory(Directory,szFile);
  }

  // Ready to open the file now..

  static ZZIP_FILE* zFile;
  char zfilename[MAX_PATH];
  unicode2ascii(szFile, zfilename, MAX_PATH);
  zFile = zzip_fopen(zfilename, "rt");
  if (!zFile) {
    StartupStore(TEXT("No topology file\n%s\n"), szFile);
    return;
  }

  TCHAR ctemp[80];
  TCHAR TempString[READLINE_LENGTH+1];
  TCHAR ShapeName[50];
  double ShapeRange;
  long ShapeIcon;
  long ShapeField;
  TCHAR wShapeFilename[MAX_PATH];
  TCHAR *Stop;
  int numtopo = 0;
  char ShapeFilename[MAX_PATH];

  while(ReadString(zFile,READLINE_LENGTH,TempString)) {

    if((_tcslen(TempString) > 0) 
       && (_tcsstr(TempString,TEXT("*")) != TempString)) // Look For Comment
      {

        BYTE red, green, blue;
        // filename,range,icon,field

        // File name
        PExtractParameter(TempString, ctemp, 0);
        _tcscpy(ShapeName, ctemp);

        _tcscpy(wShapeFilename, Directory);

        _tcscat(wShapeFilename,ShapeName);
        _tcscat(wShapeFilename,TEXT(".shp"));

#ifdef _UNICODE
        WideCharToMultiByte( CP_ACP, 0, wShapeFilename,
                             _tcslen(wShapeFilename)+1,
                             ShapeFilename,
                             200, NULL, NULL);
#else
        strcpy(ShapeFilename, wShapeFilename);
#endif

        // Shape range
        PExtractParameter(TempString, ctemp, 1);
        ShapeRange = StrToDouble(ctemp,NULL);

        // Shape icon
        PExtractParameter(TempString, ctemp, 2);
        ShapeIcon = _tcstol(ctemp, &Stop, 10);

        // Shape field for text display

        // sjt 02NOV05 - field parameter enabled
        PExtractParameter(TempString, ctemp, 3);
        if (_istalnum(ctemp[0])) {
          ShapeField = _tcstol(ctemp, &Stop, 10);
          ShapeField--;
        } else {
          ShapeField = -1;
	}

        // Red component of line / shading colour
        PExtractParameter(TempString, ctemp, 4);
        red = (BYTE)_tcstol(ctemp, &Stop, 10);

        // Green component of line / shading colour
        PExtractParameter(TempString, ctemp, 5);
        green = (BYTE)_tcstol(ctemp, &Stop, 10);

        // Blue component of line / shading colour
        PExtractParameter(TempString, ctemp, 6);
        blue = (BYTE)_tcstol(ctemp, &Stop, 10);

        if ((red==64)
            && (green==96)
            && (blue==240)) {
          // JMW update colours to ICAO standard
          red =    85; // water colours
          green = 160;
          blue =  255;
        }

        if (ShapeField<0) {
          Topology* newtopo;
          newtopo = new Topology(ShapeFilename, RGB(red,green,blue));
          topology_store[numtopo] = newtopo;
        } else {
          TopologyLabel *newtopol;
          newtopol = new TopologyLabel(ShapeFilename,
                                       RGB(red,green,blue),
                                       ShapeField);
          topology_store[numtopo] = newtopol;
        }
        if (ShapeIcon!=0)
          topology_store[numtopo]->loadBitmap(ShapeIcon);

        topology_store[numtopo]->scaleThreshold = ShapeRange;

        numtopo++;
      }
  }

  zzip_fclose(zFile);

  // file was OK, so save it
  SetRegistryString(szRegistryTopologyFile, szOrigFile);
}

