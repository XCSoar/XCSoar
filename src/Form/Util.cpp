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

#include "Form/Util.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "DataField/Base.hpp"
#include "Profile.hpp"

#include <assert.h>

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, bool value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->Set(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->SetAsFloat(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, unsigned int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->SetAsInteger(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, double value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->Set(value);
  ctl->RefreshDisplay();
}

#ifdef FIXED_MATH
void
LoadFormProperty(WndForm &form, const TCHAR *control_name, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->Set(value.as_double());
  ctl->RefreshDisplay();
}
#endif /* FIXED_MATH */

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, bool &value)
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

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, unsigned int &value)
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

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, int &value)
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

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, short &value)
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

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 bool &value, const TCHAR *registry_name)
{
  assert(control_name != NULL);
  assert(registry_name != NULL);

  const WndProperty *ctl = (const WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return false;

  bool new_value = ctl->GetDataField()->GetAsBoolean();
  if (new_value == value)
    return false;

  value = new_value;
  Profile::Set(registry_name, new_value);
  return true;
}

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, const TCHAR *reg,
                 bool &value)
{
  if (SaveFormProperty(wfm, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, const TCHAR *reg,
                 unsigned int &value)
{
  if (SaveFormProperty(wfm, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, const TCHAR *reg,
                 int &value)
{
  if (SaveFormProperty(wfm, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(WndForm *wfm, const TCHAR *field, const TCHAR *reg,
                 short &value)
{
  if (SaveFormProperty(wfm, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}
