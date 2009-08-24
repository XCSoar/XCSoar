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
#include "MapWindow.h"
#include "Topology.h"
#include "Dialogs.h"
#include "XCSoar.h"
#include "externs.h"
#include "Sizes.h"
#include "Compatibility/string.h"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "Utils.h"
#include "LogFile.hpp"

#include "SettingsUser.hpp" // for EnableTopology

#include <assert.h>

#include "wcecompat/ts_string.h"
// TODO code: check ts_string does the right thing

//////////////////////////////////////////////////


Topology* TopoStore[MAXTOPOLOGY];

extern TopologyWriter *topo_marks;

#define MINRANGE 0.2


bool RectangleIsInside(rectObj r_exterior, rectObj r_interior) {
  if ((r_interior.minx >= r_exterior.minx)&&
      (r_interior.maxx <= r_exterior.maxx)&&
      (r_interior.miny >= r_exterior.miny)&&
      (r_interior.maxy <= r_exterior.maxy))
    return true;
  else
    return false;
}

void SetTopologyBounds(const RECT rcin, const bool force) {
  static rectObj bounds_active;
  static double range_active = 1.0;
  rectObj bounds_screen;
  (void)rcin;

  bounds_screen = MapWindow::CalculateScreenBounds(1.0);

  bool recompute = false;

  // only recalculate which shapes when bounds change significantly
  // need to have some trigger for this..

  // trigger if the border goes outside the stored area
  if (!RectangleIsInside(bounds_active, bounds_screen)) {
    recompute = true;
  }

  // also trigger if the scale has changed heaps
  double range_real = max((bounds_screen.maxx-bounds_screen.minx),
			  (bounds_screen.maxy-bounds_screen.miny));
  double range = max(MINRANGE,range_real);

  double scale = range/range_active;
  if (max(scale, 1.0/scale)>4) {
    recompute = true;
  }

  if (recompute || force) {

    // make bounds bigger than screen
    if (range_real<MINRANGE) {
      scale = BORDERFACTOR*MINRANGE/range_real;
    } else {
      scale = BORDERFACTOR;
    }
    bounds_active = MapWindow::CalculateScreenBounds(scale);

    range_active = max((bounds_active.maxx-bounds_active.minx),
		       (bounds_active.maxy-bounds_active.miny));

    for (int z=0; z<MAXTOPOLOGY; z++) {
      if (TopoStore[z]) {
	TopoStore[z]->triggerUpdateCache=true;
      }
    }
    if (topo_marks) {
      topo_marks->triggerUpdateCache = true;
    }

    // now update visibility of objects in the map window
    MapWindow::ScanVisibility(&bounds_active);

  }

  // check if things have come into or out of scale limit
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      TopoStore[z]->TriggerIfScaleNowVisible();
    }
  }

  // ok, now update the caches

  if (topo_marks) {
    topo_marks->updateCache(bounds_active);
  }

  if (EnableTopology) {
    // check if any needs to have cache updates because wasnt
    // visible previously when bounds moved
    bool sneaked= false;
    bool rta;

    // we will make sure we update at least one cache per call
    // to make sure eventually everything gets refreshed

    int total_shapes_visible = 0;
    for (int z=0; z<MAXTOPOLOGY; z++) {
      if (TopoStore[z]) {
	rta = MapWindow::RenderTimeAvailable() || force || !sneaked;
	if (TopoStore[z]->triggerUpdateCache) {
	  sneaked = true;
	}
	TopoStore[z]->updateCache(bounds_active, !rta);
	total_shapes_visible += TopoStore[z]->shapes_visible_count;
      }
    }
#ifdef DEBUG_GRAPHICS
    DebugStore("%d # shapes\n", total_shapes_visible);
#endif

  }
}


void CloseTopology() {
  StartupStore(TEXT("CloseTopology\n"));

  LockTerrainDataGraphics();
  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      delete TopoStore[z];
    }
  }
  UnlockTerrainDataGraphics();
}


void DrawTopology(const HDC hdc, const RECT rc)
{
  LockTerrainDataGraphics();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    if (TopoStore[z]) {
      TopoStore[z]->Paint(hdc,rc);
    }
  }
  UnlockTerrainDataGraphics();
}



void OpenTopology() {
  StartupStore(TEXT("OpenTopology\n"));
  CreateProgressDialog(gettext(TEXT("Loading Topology File...")));

  // Start off by getting the names and paths
  static TCHAR  szOrigFile[MAX_PATH] = TEXT("\0");
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");
  static  TCHAR Directory[MAX_PATH] = TEXT("\0");

  LockTerrainDataGraphics();

  for (int z=0; z<MAXTOPOLOGY; z++) {
    TopoStore[z] = 0;
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
      UnlockTerrainDataGraphics();
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
    UnlockTerrainDataGraphics();
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

    if(_tcslen(TempString) > 0 && _tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
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
          TopoStore[numtopo] = newtopo;
        } else {
          TopologyLabel *newtopol;
          newtopol = new TopologyLabel(ShapeFilename,
                                       RGB(red,green,blue),
                                       ShapeField);
          TopoStore[numtopo] = newtopol;
        }
        if (ShapeIcon!=0)
          TopoStore[numtopo]->loadBitmap(ShapeIcon);

        TopoStore[numtopo]->scaleThreshold = ShapeRange;

        numtopo++;
      }
  }

  //  CloseHandle (hFile);
  zzip_fclose(zFile);

  // file was OK, so save it
  SetRegistryString(szRegistryTopologyFile, szOrigFile);

  UnlockTerrainDataGraphics();

}
