/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "TimeConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/Float.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;
static bool loading = false;


static void
SetLocalTime(int utc_offset)
{
  WndProperty* wp;
  TCHAR temp[20];
  int time(XCSoarInterface::Basic().time);
  Units::TimeToTextHHMMSigned(temp, TimeLocal(time, utc_offset));

  wp = (WndProperty*)wf->FindByName(_T("prpLocalTime"));
  assert(wp != NULL);

  wp->SetText(temp);
  wp->RefreshDisplay();
}


void
TimeConfigPanel::OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch(Mode) {
  case DataField::daChange:
  {
    DataFieldFloat &df = *(DataFieldFloat *)Sender;
    int ival = iround(df.GetAsFixed() * 3600);
    SetLocalTime(ival);
    break;
  }
  case DataField::daSpecial:
    return;
  }
}


void
TimeConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  loading = true;

  int utc_offset = XCSoarInterface::SettingsComputer().utc_offset;
  LoadFormProperty(*wf, _T("prpUTCOffset"),
                   fixed(iround(fixed(utc_offset) / 1800)) / 2);
#ifdef WIN32
  if (IsEmbedded() && !IsAltair())
    ((WndProperty*)wf->FindByName(_T("prpUTCOffset")))->set_enabled(false);
#endif
  SetLocalTime(utc_offset);

  loading = false;
}


bool
TimeConfigPanel::Save()
{
  bool changed = false;

  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  int ival = iround(GetFormValueFixed(*wf, _T("prpUTCOffset")) * 3600);
  if (settings_computer.utc_offset != ival) {
    settings_computer.utc_offset = ival;

    // have to do this because registry variables can't be negative!
    if (ival < 0)
      ival += 24 * 3600;

    Profile::Set(szProfileUTCOffset, ival);
    changed = true;
  }

  return changed;
}
