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

#include "Topology/TopologyGlue.hpp"
#include "Topology/TopologyStore.hpp"
#include "Language.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "ProgressGlue.hpp"
#include "IO/ZipLineReader.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"

bool
LoadConfiguredTopology(TopologyStore &store)
{
  LogStartUp(_T("Loading Topology File..."));
  ProgressGlue::Create(_("Loading Topology File..."));

  // Start off by getting the names and paths
  TCHAR szFile[MAX_PATH];

  if (!Profile::GetPath(szProfileTopologyFile, szFile) ||
      !File::Exists(szFile)) {
    // file is blank, so look for it in a map file
    if (!Profile::GetPath(szProfileMapFile, szFile) ||
        !File::Exists(szFile))
      return false;

    // Look for the file within the map zip file...
    _tcscat(szFile, _T("/"));
    _tcscat(szFile, _T("topology.tpl"));
  }

  // Ready to open the file now..
  ZipLineReaderA reader(szFile);
  if (reader.error()) {
    LogStartUp(_T("No topology file: %s"), szFile);
    return false;
  }

  TCHAR buffer[MAX_PATH];
  const TCHAR *Directory = DirName(szFile, buffer);
  if (Directory == NULL)
    return false;

  store.Load(reader, Directory);
  return true;
}
