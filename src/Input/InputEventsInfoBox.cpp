/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "InputEvents.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Util/StringUtil.hpp"

// SelectInfoBox
// Selects the next or previous infobox
void
InputEvents::eventSelectInfoBox(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("next")))
    InfoBoxManager::Event_Select(1);
  else if (StringIsEqual(misc, _T("previous")))
    InfoBoxManager::Event_Select(-1);
}

// ChangeInfoBoxType
// Changes the type of the current infobox to the next/previous type
void
InputEvents::eventChangeInfoBoxType(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("next")))
    InfoBoxManager::Event_Change(1);
  else if (StringIsEqual(misc, _T("previous")))
    InfoBoxManager::Event_Change(-1);
}
