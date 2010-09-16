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

#include "Pages.hpp"

#include "Interface.hpp"
#include "MainWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Profile.hpp"

#include <tchar.h>

namespace Pages
{
  int Current = 0;
  PageLayout pages[8] = {
    lMapInfoBoxes,
    lMapAuxInfoBoxes,
    lMap,
    lEmpty,
    lEmpty,
    lEmpty,
    lEmpty,
    lEmpty
  };

  void Update();

  bool MakeProfileKey(TCHAR* buffer, int page);
  bool MakeProfileValue(TCHAR* buffer, int page);
  bool ParseProfileValue(TCHAR* buffer, int page);
}

void
Pages::Update()
{
  OpenLayout(pages[Current]);
}

void
Pages::Next()
{
  Current++;
  if (Current > 7 || pages[Current] == lEmpty)
    Current = 0;

  Update();
}

void
Pages::Prev()
{
  Current--;
  if (Current < 0)
    Current = 7;

  while (pages[Current] == lEmpty && Current > 0)
    Current--;

  Update();
}

void
Pages::Open(int page)
{
  if (page < 0 || page > 7)
    return;

  Current = page;
  Update();
}

void
Pages::OpenLayout(PageLayout layout)
{
  switch (layout) {
  case lMapInfoBoxes:
    XCSoarInterface::main_window.SetFullScreen(false);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
    break;

  case lMapAuxInfoBoxes:
    XCSoarInterface::main_window.SetFullScreen(false);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = true;
    break;

  case lMap:
    XCSoarInterface::main_window.SetFullScreen(true);
    XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
    break;
  }

  InfoBoxManager::SetDirty();
  XCSoarInterface::SendSettingsMap(true);
}

void
Pages::SetLayout(int page, PageLayout layout)
{
  if (page < 0 || page > 7)
    return;

  pages[page] = layout;

  if (page == Current)
    Update();
}

Pages::PageLayout
Pages::GetLayout(int page)
{
  if (page < 0 || page > 7)
    return lEmpty;

  return pages[page];
}

bool
Pages::MakeProfileKey(TCHAR* buffer, int page)
{
  if (page < 0 || page > 7)
    return false;

  _tcscpy(buffer, CONF("Page"));
  _stprintf(buffer + _tcslen(buffer), _T("%d"), page + 1);
  return true;
}

bool
Pages::MakeProfileValue(TCHAR* buffer, int page)
{
  if (page < 0 || page > 7)
    return false;

  switch (pages[page]) {
  case lMapInfoBoxes:
    _tcscpy(buffer, _T("map ib_normal"));
    break;

  case lMapAuxInfoBoxes:
    _tcscpy(buffer, _T("map ib_aux"));
    break;

  case lMap:
    _tcscpy(buffer, _T("map"));
    break;

  case lEmpty:
    _tcscpy(buffer, _T(""));
    break;

  default:
    return false;
  }

  return true;
}

bool
Pages::ParseProfileValue(TCHAR* buffer, int page)
{
  if (page < 0 || page > 7)
    return false;

  if (_tcscmp(buffer, _T("map ib_normal")) == 0)
    SetLayout(page, lMapInfoBoxes);
  else if (_tcscmp(buffer, _T("map ib_aux")) == 0)
    SetLayout(page, lMapAuxInfoBoxes);
  else if (_tcscmp(buffer, _T("map")) == 0)
    SetLayout(page, lMap);
  else {
    SetLayout(page, lEmpty);
    return false;
  }

  return true;
}

void
Pages::SaveToProfile()
{
  for (int i = 0; i < 8; i++) {
    TCHAR key[64] = _T("");
    TCHAR value[255] = _T("");

    if (!MakeProfileKey(key, i) || !MakeProfileValue(value, i))
      continue;

    Profile::Set(key, value);
  }
}

void
Pages::LoadFromProfile()
{
  for (int i = 0; i < 8; i++) {
    TCHAR key[64] = _T("");
    TCHAR value[255] = _T("");

    if (!MakeProfileKey(key, i) || !Profile::Get(key, value, 255))
      continue;

    ParseProfileValue(value, i);
  }
}
