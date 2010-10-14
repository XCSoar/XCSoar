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

#include "Topology/TopologyStore.hpp"
#include "Topology/TopologyFile.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Compatibility/string.h"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "SettingsMap.hpp" // for EnableTopology
#include "IO/ZipLineReader.hpp"
#include "ProgressGlue.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

#include <assert.h>

void
TopologyStore::ScanVisibility(const Projection &m_projection)
{
  // check if any needs to have cache updates because wasnt
  // visible previously when bounds moved

  // we will make sure we update at least one cache per call
  // to make sure eventually everything gets refreshed
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    if (topology_store[z]) {
      topology_store[z]->updateCache(m_projection);
    }
  }
}

TopologyStore::~TopologyStore()
{
  Close();
}

void
TopologyStore::Close()
{
  LogStartUp(_T("CloseTopology"));

  Reset();
}

/**
 * Draws the topology to the given canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void
TopologyStore::Draw(Canvas &canvas, BitmapCanvas &bitmap_canvas,
                    const Projection &projection) const
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    if (topology_store[z])
      topology_store[z]->Paint(canvas, bitmap_canvas, projection);
  }
}

void
TopologyStore::DrawLabels(Canvas &canvas, const Projection &projection,
                          LabelBlock &label_block,
                          const SETTINGS_MAP &settings_map) const
{
  for (unsigned i = 0; i < MAXTOPOLOGY; i++)
    if (topology_store[i] != NULL)
      topology_store[i]->PaintLabels(canvas, projection,
                                     label_block, settings_map);
}

TopologyStore::TopologyStore()
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    topology_store[z] = NULL;
  }

  Open();
}

void
TopologyStore::Reload()
{
  Close();
  Open();
}

void
TopologyStore::Open()
{
  LogStartUp(_T("OpenTopology"));

  ProgressGlue::Create(_("Loading Topology File..."));

  // Start off by getting the names and paths
  TCHAR szFile[MAX_PATH];

  if (!Profile::GetPath(szProfileTopologyFile, szFile) ||
      !File::Exists(szFile)) {
    // file is blank, so look for it in a map file
    if (!Profile::GetPath(szProfileMapFile, szFile) ||
        !File::Exists(szFile))
      return;

    // Look for the file within the map zip file...
    _tcscat(szFile, _T("/"));
    _tcscat(szFile, _T("topology.tpl"));
  }

  // Ready to open the file now..
  ZipLineReaderA reader(szFile);
  if (reader.error()) {
    LogStartUp(_T("No topology file: %s"), szFile);
    return;
  }

  TCHAR buffer[MAX_PATH];
  const TCHAR *Directory = DirName(szFile, buffer);
  if (Directory != NULL)
    Load(reader, Directory);
}

void
TopologyStore::Load(NLineReader &reader, const TCHAR* Directory)
{
  Reset();

  double ShapeRange;
  long ShapeIcon;
  long ShapeField;
  int numtopo = 0;
  char ShapeFilename[MAX_PATH];

#ifdef _UNICODE
  if (WideCharToMultiByte(CP_ACP, 0, Directory, -1, ShapeFilename, MAX_PATH,
                          NULL, NULL) <= 0)
    return;
#else
  strcpy(ShapeFilename, Directory);
#endif
  strcat(ShapeFilename, DIR_SEPARATOR_S);

  char *ShapeFilenameEnd = ShapeFilename + strlen(ShapeFilename);

  char *line;
  while ((line = reader.read()) != NULL) {
    // Look For Comment
    if (string_is_empty(line) || line[0] == '*')
      continue;

    BYTE red, green, blue;
    // filename,range,icon,field

    // File name
    char *p = strchr(line, ',');
    if (p == NULL || p == line)
      continue;

    memcpy(ShapeFilenameEnd, line, p - line);
    strcpy(ShapeFilenameEnd + (p - line), ".shp");

    // Shape range
    ShapeRange = strtod(p + 1, &p);
    if (*p != _T(','))
      continue;

    // Shape icon
    ShapeIcon = strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Shape field for text display
    // sjt 02NOV05 - field parameter enabled
    ShapeField = strtol(p + 1, &p, 10) - 1;
    if (*p != _T(','))
      continue;

    // Red component of line / shading colour
    red = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Green component of line / shading colour
    green = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Blue component of line / shading colour
    blue = (BYTE)strtol(p + 1, NULL, 10);

    if ((red == 64) && (green == 96) && (blue == 240)) {
      // JMW update colours to ICAO standard
      red = 85; // water colours
      green = 160;
      blue = 255;
    }

    topology_store[numtopo] = new TopologyFile(ShapeFilename,
                                               Color(red, green, blue),
                                               ShapeField);

    if (ShapeIcon != 0)
      topology_store[numtopo]->loadIcon(ShapeIcon);

    topology_store[numtopo]->scaleThreshold = ShapeRange;

    numtopo++;
  }
}

void
TopologyStore::Reset()
{
  for (int z = 0; z < MAXTOPOLOGY; z++) {
    delete topology_store[z];
    topology_store[z] = NULL;
  }
}
