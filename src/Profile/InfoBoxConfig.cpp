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

#include "Profile/InfoBoxConfig.hpp"

#include "Profile/Profile.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Sizes.h"

// This function checks to see if Final Glide mode infoboxes have been
// initialised.  If all are zero, then the current configuration was
// using XCSoarV3 infoboxes, so copy settings from cruise mode.
static void
CheckInfoTypes()
{
  if (InfoBoxManager::IsEmpty(InfoBoxManager::MODE_CRUISE))
    return;

  bool iszero_fg = InfoBoxManager::IsEmpty(InfoBoxManager::MODE_FINAL_GLIDE);
  bool iszero_aux = InfoBoxManager::IsEmpty(InfoBoxManager::MODE_AUXILIARY);
  if (!iszero_fg && !iszero_aux)
    return;

  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i) {
    if (iszero_fg)
      InfoBoxManager::SetType(i, InfoBoxManager::GetType(i, InfoBoxManager::MODE_CRUISE),
                              InfoBoxManager::MODE_FINAL_GLIDE);
    if (iszero_aux)
      InfoBoxManager::SetType(i, InfoBoxManager::GetType(i, InfoBoxManager::MODE_CRUISE),
                              InfoBoxManager::MODE_AUXILIARY);
  }
}

void
Profile::SetInfoBoxes(int Index, int the_type)
{
  Set(szProfileDisplayType[Index], the_type);
}

void
Profile::LoadInfoBoxes()
{
  unsigned Temp = 0;

  for (unsigned i = 0; i < MAXINFOWINDOWS; i++) {
    if (Get(szProfileDisplayType[i], Temp))
      InfoBoxManager::SetTypes(i, Temp);
  }

  // check against V3 infotypes
  CheckInfoTypes();
}
