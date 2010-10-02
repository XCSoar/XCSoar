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
#include "Profile/Profile.hpp"
#include "Language.hpp"

#include <stdio.h>

namespace Pages
{
  int Current = 0;
  PageLayout pages[8];

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
  if (Current > 7 || pages[Current].Type == PageLayout::t_Empty)
    Current = 0;

  Update();
}

void
Pages::Prev()
{
  Current--;
  if (Current < 0)
    Current = 7;

  while (pages[Current].Type == PageLayout::t_Empty && Current > 0)
    Current--;

  Update();
}

void
Pages::Open(int page)
{
  if (page < 0 || page > 7)
    return;

  if (pages[page].Type == PageLayout::t_Empty)
    return;

  Current = page;
  Update();
}

void
Pages::OpenLayout(PageLayout &layout)
{
  switch (layout.Type) {
  case PageLayout::t_Map:
    switch (layout.MapInfoBoxes) {
    case PageLayout::mib_Normal:
      XCSoarInterface::main_window.SetFullScreen(false);
      XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
      break;

    case PageLayout::mib_Aux:
      XCSoarInterface::main_window.SetFullScreen(false);
      XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = true;
      break;

    case PageLayout::mib_None:
    default:
      XCSoarInterface::main_window.SetFullScreen(true);
      XCSoarInterface::SetSettingsMap().EnableAuxiliaryInfo = false;
      break;
    }
    break;

  default:
    return;
  }

  InfoBoxManager::SetDirty();
  XCSoarInterface::SendSettingsMap(true);
}

void
Pages::SetLayout(int page, PageLayout &layout)
{
  if (page < 0 || page > 7)
    return;

  pages[page] = layout;

  if (page == Current)
    Update();
}

Pages::PageLayout*
Pages::GetLayout(int page)
{
  if (page < 0 || page > 7)
    return NULL;

  return &pages[page];
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

void
Pages::PageLayout::MakeTitle(TCHAR* buffer)
{
  switch (Type) {
  case t_Map:
    _tcscpy(buffer, _("Map"));

    switch (MapInfoBoxes) {
    case mib_Normal:
      _tcscat(buffer, _T(" "));
      _tcscat(buffer, _("w. infoboxes"));
      break;

    case mib_Aux:
      _tcscat(buffer, _T(" "));
      _tcscat(buffer, _("w. aux. infoboxes"));
      break;
    }
    break;

  case t_Empty:
  default:
    _tcscpy(buffer, _T("---"));
    break;
  }
}

void
Pages::PageLayout::MakeConfigString(TCHAR* buffer)
{
  switch (Type) {
  case t_Map:
    _tcscpy(buffer, _T("map"));

    switch (MapInfoBoxes) {
    case mib_Normal:
      _tcscat(buffer, _T(" ib_normal"));
      break;

    case mib_Aux:
      _tcscat(buffer, _T(" ib_aux"));
      break;
    }
    break;

  case t_Empty:
  default:
    _tcscpy(buffer, _T(""));
    break;
  }
}

bool
Pages::MakeProfileValue(TCHAR* buffer, int page)
{
  PageLayout* pl = GetLayout(page);
  if (!pl)
    return false;

  pl->MakeConfigString(buffer);

  return true;
}

void
Pages::PageLayout::ParseConfigString(TCHAR* buffer)
{
  if (_tcsncmp(buffer, _T("map"), 3) == 0) {
    Type = t_Map;

    if (_tcsncmp(buffer + 3, _T(" ib_normal"), 10) == 0)
      MapInfoBoxes = mib_Normal;
    else if (_tcsncmp(buffer + 3, _T(" ib_aux"), 7) == 0)
      MapInfoBoxes = mib_Aux;
    else
      MapInfoBoxes = mib_None;
  } else {
    Type = t_Empty;
  }
}

bool
Pages::ParseProfileValue(TCHAR* buffer, int page)
{
  if (page < 0 || page > 7)
    return false;

  pages[page].ParseConfigString(buffer);
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
  LoadDefault();

  for (int i = 0; i < 8; i++) {
    TCHAR key[64] = _T("");
    TCHAR value[255] = _T("");

    if (!MakeProfileKey(key, i) || !Profile::Get(key, value, 255))
      continue;

    ParseProfileValue(value, i);
  }

  Update();
}

void
Pages::LoadDefault()
{
  pages[0].Type = PageLayout::t_Map;
  pages[0].MapInfoBoxes = PageLayout::mib_Normal;

  pages[1].Type = PageLayout::t_Map;
  pages[1].MapInfoBoxes = PageLayout::mib_Aux;

  pages[2].Type = PageLayout::t_Map;
  pages[2].MapInfoBoxes = PageLayout::mib_None;

  pages[3].Type = PageLayout::t_Empty;
  pages[4].Type = PageLayout::t_Empty;
  pages[5].Type = PageLayout::t_Empty;
  pages[6].Type = PageLayout::t_Empty;
  pages[7].Type = PageLayout::t_Empty;
}
