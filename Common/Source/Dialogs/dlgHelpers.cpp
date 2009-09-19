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

#include "Dialogs/dlgHelpers.hpp"
#include "Registry.hpp"
#include "WindowControls.h"
#include "DataField/Base.hpp"

bool SetValueRegistryOnChange(WndForm* wfm, 
			      const TCHAR* field,
			      const TCHAR* reg,
			      bool &value) {
  if (SetValueOnChange(wfm, field, value)) {
    SetToRegistry(reg, (DWORD)value);
    return true;
  } else {
    return false;
  }
}


bool SetValueRegistryOnChange(WndForm* wfm, 
			      const TCHAR* field,
			      const TCHAR* reg,
			      unsigned int &value) {
  if (SetValueOnChange(wfm, field, value)) {
    SetToRegistry(reg, (DWORD)value);
    return true;
  } else {
    return false;
  }
}


bool SetValueRegistryOnChange(WndForm* wfm, 
			      const TCHAR* field,
			      const TCHAR* reg,
			      int &value) {
  if (SetValueOnChange(wfm, field, value)) {
    SetToRegistry(reg, (DWORD)value);
    return true;
  } else {
    return false;
  }
}

bool SetValueRegistryOnChange(WndForm* wfm, 
			      const TCHAR* field,
			      const TCHAR* reg,
			      DisplayTextType_t &value) {
  if (SetValueOnChange(wfm, field, value)) {
    SetToRegistry(reg, (DWORD)value);
    return true;
  } else {
    return false;
  }
}

bool SetValueRegistryOnChange(WndForm* wfm, 
			      const TCHAR* field,
			      const TCHAR* reg,
			      short &value) {
  if (SetValueOnChange(wfm, field, value)) {
    SetToRegistry(reg, (DWORD)value);
    return true;
  } else {
    return false;
  }
}

////


bool SetValueOnChange(WndForm* wfm, const TCHAR* field, bool &value) 
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if (value != wp->GetDataField()->GetAsBoolean()) {
      value = wp->GetDataField()->GetAsBoolean();
      return true;
    }
  }
  return false;
}


bool SetValueOnChange(WndForm* wfm, const TCHAR* field, unsigned int &value) 
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if ((int)value != wp->GetDataField()->GetAsInteger()) {
      value = wp->GetDataField()->GetAsInteger();
      return true;
    }
  }
  return false;
}


bool SetValueOnChange(WndForm* wfm, const TCHAR* field, int &value) 
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if (value != wp->GetDataField()->GetAsInteger()) {
      value = wp->GetDataField()->GetAsInteger();
      return true;
    }
  }
  return false;
}

bool SetValueOnChange(WndForm* wfm, const TCHAR* field, DisplayTextType_t &value) 
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if ((int)value != wp->GetDataField()->GetAsInteger()) {
      value = (DisplayTextType_t)wp->GetDataField()->GetAsInteger();
      return true;
    }
  }
  return false;
}

bool SetValueOnChange(WndForm* wfm, const TCHAR* field, short &value) 
{
  WndProperty* wp = (WndProperty*)wfm->FindByName(field);
  if (wp) {
    if (value != wp->GetDataField()->GetAsInteger()) {
      value = wp->GetDataField()->GetAsInteger();
      return true;
    }
  }
  return false;
}

