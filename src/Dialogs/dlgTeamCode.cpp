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

#include "Dialogs/Internal.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "StringUtil.hpp"
#include "TeamCodeCalculation.h"
#include "Compiler.h"

#include <stdio.h>

static WndForm *wf = NULL;

static void
Update()
{
  WndProperty* wp;
  TCHAR Text[100];
  Angle teammateBearing = XCSoarInterface::Calculated().TeammateBearing;
  double teammateRange = XCSoarInterface::Calculated().TeammateRange;

  if (XCSoarInterface::SettingsComputer().TeamCodeRefWaypoint >= 0) {
    double Value = (teammateBearing - XCSoarInterface::Basic().TrackBearing).
      as_delta().value_degrees();

    if (Value > 1)
      _stprintf(Text, _T("%2.0f")_T(DEG)_T(">"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("<%2.0f")_T(DEG), -Value);
    else
      _tcscpy(Text, _T("<>"));
  } else {
    _tcscpy(Text, _T("---"));
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRelBearing"));
  if (wp) {
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpBearing"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(teammateBearing.value_degrees());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserDistance(fixed(teammateRange)));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpOwnCode"));
  if (wp) {
    _tcsncpy(Text, XCSoarInterface::Calculated().OwnTeamCode, 5);
    Text[5] = '\0';
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMateCode"));
  if (wp) {
    wp->SetText(XCSoarInterface::SettingsComputer().TeammateCode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlarmLock"));
  if (wp) {
    wp->SetText(XCSoarInterface::SettingsComputer().TeamFlarmTracking ?
                XCSoarInterface::SettingsComputer().TeamFlarmCNTarget : _T(""));
    wp->RefreshDisplay();
  }
}

static void
OnCodeClicked(gcc_unused WndButton &button)
{
  TCHAR newTeammateCode[10];

  _tcsncpy(newTeammateCode,
           XCSoarInterface::SettingsComputer().TeammateCode, 10);

  if (!dlgTextEntryShowModal(newTeammateCode, 7))
    return;

  for (int i = _tcslen(newTeammateCode) - 1; i >= 0; i--) {
    if (newTeammateCode[i] != _T(' '))
      break;

    newTeammateCode[i] = 0;
  }

  _tcsncpy(XCSoarInterface::SetSettingsComputer().TeammateCode,
           newTeammateCode, 10);
  if (!string_is_empty(XCSoarInterface::SettingsComputer().TeammateCode)) {
    XCSoarInterface::SetSettingsComputer().TeammateCodeValid = true;
    XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = false;
  }
  else
    XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;
}

static void
OnFlarmLockClicked(gcc_unused WndButton &button)
{
  TCHAR newTeamFlarmCNTarget[4];
  _tcsncpy(newTeamFlarmCNTarget,
           XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget, 4);

  if (!dlgTextEntryShowModal(newTeamFlarmCNTarget, 4))
    return;

  _tcsncpy(XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget,
           newTeamFlarmCNTarget, 4);
  XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;

  if (string_is_empty(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget)) {
    XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = false;
    XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget.clear();
    return;
  }

  FlarmId flarmId = FlarmDetails::LookupId(
      XCSoarInterface::SettingsComputer().TeamFlarmCNTarget);

  if (!flarmId.defined()) {
    MessageBoxX(_("Unknown Competition Number"),
                _("Not Found"), MB_OK | MB_ICONINFORMATION);

    XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = false;
    XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget.clear();
    XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
  } else {
    XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = flarmId;
    XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = true;
  }
}

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  Update();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(OnFlarmLockClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgTeamCodeShowModal(void)
{
  WndProperty* wp = NULL;
  WndButton *buttonCode = NULL;

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable,
        XCSoarInterface::main_window, _T("IDR_XML_TEAMCODE_L"));
  else
    wf = LoadDialog(CallBackTable,
        XCSoarInterface::main_window, _T("IDR_XML_TEAMCODE"));

  if (!wf)
    return;

  // set event for button
  buttonCode = ((WndButton *)wf->FindByName(_T("cmdSetCode")));
  if (buttonCode) {
    buttonCode->SetOnClickNotify(OnCodeClicked);
  }

  // Set unit for range
  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
  }

  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;
}
