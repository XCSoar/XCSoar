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

#include "InfoBoxes/Content/Thermal.hpp"
#include "InfoBoxes/Content/MacCready.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Interface.hpp"

#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"

#include <tchar.h>
#include <stdio.h>

static void
SetVSpeed(InfoBoxWindow &infobox, fixed value)
{
  TCHAR buffer[32];
  Units::FormatUserVSpeed(value, buffer, 32, false);
  infobox.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  infobox.SetValueUnit(Units::Current.VerticalSpeedUnit);
}

/*
 * InfoBoxContentMacCready
 *
 * Subpart Panel Edit
 */

static int InfoBoxID;

Window*
InfoBoxContentMacCready::PnlEditLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                 WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessEdit =
      LoadWindow(dlgContent.CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXMACCREADYEDIT"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}

void
InfoBoxContentMacCready::PnlEditOnCloseClicked(gcc_unused WndButton &Sender)
{
  dlgInfoBoxAccess::OnClose();
}

void
InfoBoxContentMacCready::PnlEditOnPlusSmall(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.1"));
}

void
InfoBoxContentMacCready::PnlEditOnPlusBig(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.5"));
}

void
InfoBoxContentMacCready::PnlEditOnMinusSmall(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.1"));
}

void
InfoBoxContentMacCready::PnlEditOnMinusBig(gcc_unused WndButton &Sender)
{
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.5"));
}

/*
 * Subpart Panel Setup
 */

Window*
InfoBoxContentMacCready::PnlSetupLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                 WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
      LoadWindow(dlgContent.CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXMACCREADYSETUP"));
  assert(wInfoBoxAccessSetup);

  return wInfoBoxAccessSetup;
}

bool
InfoBoxContentMacCready::PnlSetupPreShow(TabBarControl::EventType EventType)
{

  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    ((WndButton *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("cmdMode")))->SetCaption(_("MANUAL"));
  else
    ((WndButton *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("cmdMode")))->SetCaption(_("AUTO"));

  return true;
}

void
InfoBoxContentMacCready::PnlSetupOnSetup(gcc_unused WndButton &Sender) {
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

void
InfoBoxContentMacCready::PnlSetupOnMode(gcc_unused WndButton &Sender)
{
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    Sender.SetCaption(_("AUTO"));
  else
    Sender.SetCaption(_("MANUAL"));

  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("mode"));
}


/*
 * Subpart callback function pointers
 */


InfoBoxContentMacCready::PanelContent InfoBoxContentMacCready::Panels[] = {
  InfoBoxContentMacCready::PanelContent (
    _("Edit"),
    (*InfoBoxContentMacCready::PnlEditLoad)),

  InfoBoxContentMacCready::PanelContent (
    _("Setup"),
    (*InfoBoxContentMacCready::PnlSetupLoad),
    NULL,
    (*InfoBoxContentMacCready::PnlSetupPreShow))
};

CallBackTableEntry InfoBoxContentMacCready::CallBackTable[] = {
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnPlusSmall),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnPlusBig),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnMinusSmall),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnMinusBig),

  DeclareCallBackEntry(InfoBoxContentMacCready::PnlSetupOnSetup),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlSetupOnMode),

  DeclareCallBackEntry(NULL)
};

InfoBoxContentMacCready::DialogContent InfoBoxContentMacCready::dlgContent = {
    InfoBoxContentMacCready::PANELSIZE,
    InfoBoxContentMacCready::Panels,
    InfoBoxContentMacCready::CallBackTable
};

InfoBoxContentMacCready::DialogContent*
InfoBoxContentMacCready::GetDialogContent() {
  return &dlgContent;
}

/*
 * Subpart normal operations
 */

void
InfoBoxContentMacCready::Update(InfoBoxWindow &infobox)
{
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();

  SetVSpeed(infobox, settings_computer.glide_polar_task.GetMC());

  // Set Comment
  if (XCSoarInterface::SettingsComputer().task.auto_mc)
    infobox.SetComment(_("AUTO"));
  else
    infobox.SetComment(_("MANUAL"));
}

bool
InfoBoxContentMacCready::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (protected_task_manager == NULL)
    return false;

  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();
  const GlidePolar &polar = settings_computer.glide_polar_task;
  TaskBehaviour &task_behaviour = CommonInterface::SetSettingsComputer().task;
  fixed mc = polar.GetMC();

  switch (keycode) {
  case ibkUp:
    mc = std::min(mc + fixed_one / 10, fixed(5));
    ActionInterface::SetMacCready(mc);
    task_behaviour.auto_mc = false;
    Profile::Set(szProfileAutoMc, false);
    return true;

  case ibkDown:
    mc = std::max(mc - fixed_one / 10, fixed_zero);
    ActionInterface::SetMacCready(mc);
    task_behaviour.auto_mc = false;
    Profile::Set(szProfileAutoMc, false);
    return true;

  case ibkLeft:
    task_behaviour.auto_mc = false;
    Profile::Set(szProfileAutoMc, false);
    return true;

  case ibkRight:
    task_behaviour.auto_mc = true;
    Profile::Set(szProfileAutoMc, true);
    return true;

  case ibkEnter:
    task_behaviour.auto_mc = !task_behaviour.auto_mc;
    Profile::Set(szProfileAutoMc, task_behaviour.auto_mc);
    return true;
  }
  return false;
}

bool
InfoBoxContentMacCready::HandleQuickAccess(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return false;

  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();
  const GlidePolar &polar = settings_computer.glide_polar_task;
  fixed mc = polar.GetMC();

  if (_tcscmp(misc, _T("+0.1")) == 0) {
    return HandleKey(ibkUp);

  } else if (_tcscmp(misc, _T("+0.5")) == 0) {
    mc = std::min(mc + fixed_one / 2, fixed(5));
    ActionInterface::SetMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("-0.1")) == 0) {
    return HandleKey(ibkDown);

  } else if (_tcscmp(misc, _T("-0.5")) == 0) {
    mc = std::max(mc - fixed_one / 2, fixed_zero);
    ActionInterface::SetMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("mode")) == 0) {
    return HandleKey(ibkEnter);
  }

  return false;
}
