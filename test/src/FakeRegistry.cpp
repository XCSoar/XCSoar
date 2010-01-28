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

#include "Registry.hpp"

const TCHAR szRegistryWayPointFile[] = _T("");
const TCHAR szRegistryAdditionalWayPointFile[] = _T("");
const TCHAR szRegistryWaypointsOutOfRange[] = _T("");
const TCHAR szRegistryMapFile[] = _T("");
const TCHAR szRegistryAlternate1[] = _T("");
const TCHAR szRegistryAlternate2[] = _T("");
const TCHAR szRegistryHomeWaypoint[] = _T("");
const TCHAR szRegistryTeamcodeRefWaypoint[] = _T("");

bool
GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
  return false;
}

HRESULT
SetToRegistry(const TCHAR *szRegValue, int nVal)
{
  return 0;
}

HRESULT
SetToRegistry(const TCHAR *szRegValue, unsigned nVal)
{
  return 0;
}

HRESULT
SetToRegistry(const TCHAR *szRegValue, unsigned long nVal)
{
  return 0;
}

bool
SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos)
{
  return 0;
}
