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

#ifndef XCSOAR_LOCAL_PATH_HPP
#define XCSOAR_LOCAL_PATH_HPP

#include <tchar.h>

#define XCSDATADIR _T("XCSoarData")

/**
 * Overrides the detected primary data path.
 */
void
SetPrimaryDataPath(const TCHAR *path);

/**
 * Returns the absolute path of the primary data directory.
 */
const TCHAR *
GetPrimaryDataPath();

/**
 * Returns the location of XCSoarData in the user's home directory.
 *
 * @return a buffer which may be used to build the path
 */
const TCHAR *
GetHomeDataPath(TCHAR *buffer);

/**
 * Returns the path of the XCSoarData folder, optionally including
 * the given file name
 * @param buffer Output buffer
 * @param file optional filename to include in the output
 */
void LocalPath(TCHAR* buf, const TCHAR *file);

/**
 * Converts a file path by replacing %LOCAL_PATH% with the full pathname to
 * the XCSoarData folder
 * @param filein Pointer to the string to convert
 */
void ExpandLocalPath(TCHAR* filein);

/**
 * Converts a file path from full pathname to a shorter version with the
 * XCSoarData folder replaced by %LOCAL_PATH%
 * @param filein Pointer to the string to convert
 */
void ContractLocalPath(TCHAR* filein);

#endif
