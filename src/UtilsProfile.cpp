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

#include "UtilsText.hpp"
#include "Registry.hpp"
#include "Profile.hpp"
#include "Protection.hpp"
#include "Asset.hpp"
#include "Engine/Math/fixed.hpp"

#include <assert.h>

int
propGetScaleList(fixed *List, size_t Size)
{
  static const TCHAR Name[] = TEXT("ScaleList");
  TCHAR Buffer[128];
  TCHAR *pWClast, *pToken;
  int Idx = 0;
  double vlast = 0;
  double val;

  assert(List != NULL);
  assert(Size > 0);

  SetRegistryString(Name, TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (!GetRegistryString(Name, Buffer, sizeof(Buffer) / sizeof(TCHAR)))
    return 0;

  pToken = _tcstok_r(Buffer, TEXT(","), &pWClast);

  while (Idx < (int)Size && pToken != NULL) {
    val = _tcstod(pToken, NULL);
    if (Idx > 0) {
      List[Idx] = (val + vlast) / 2;
      Idx++;
    }
    List[Idx] = val;
    Idx++;
    vlast = val;
    pToken = _tcstok_r(NULL, TEXT(","), &pWClast);
  }

  return Idx;
}
