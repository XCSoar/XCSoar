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

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "DataField/Float.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"
#include "LoggerConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Plane/PlaneGlue.hpp"

static WndForm* wf = NULL;
static WndButton *buttonPilotName = NULL;
static WndButton *buttonAircraftType = NULL;
static WndButton *buttonAircraftReg = NULL;
static WndButton *buttonCompetitionId = NULL;
static WndButton *buttonLoggerID = NULL;
static bool changed = false;

static void
UpdateButtons(void)
{
  TCHAR text[120];
  StaticString<100> val;

  const Plane &plane = CommonInterface::SettingsComputer().plane;

  if (!Profile::Get(szProfilePilotName, val) || val.empty())
    val = _("(blank)");

  _stprintf(text, _T("%s: %s"), _("Pilot name"), val.c_str());
  buttonPilotName->SetCaption(text);

  val = plane.type;
  if (val.empty())
    val = _("(blank)");

  _stprintf(text, _T("%s: %s"), _("Aircraft type"), val.c_str());
  buttonAircraftType->SetCaption(text);

  val = plane.registration;
  if (val.empty())
    val = _("(blank)");

  _stprintf(text, _T("%s: %s"), _("Aircraft reg."), val.c_str());
  buttonAircraftReg->SetCaption(text);

  val = plane.competition_id;
  if (val.empty())
    val = _("(blank)");

  _stprintf(text, _T("%s: %s"), _("Competition ID"), val.c_str());
  buttonCompetitionId->SetCaption(text);

  if (!Profile::Get(szProfileLoggerID, val) || val.empty())
    val = _("(blank)");

  _stprintf(text, _T("%s: %s"), _("Logger ID"), val.c_str());
  buttonLoggerID->SetCaption(text);
}


static void
OnCompetitionIdClicked(gcc_unused WndButton &button)
{
  Plane &plane = CommonInterface::SetSettingsComputer().plane;

  if (dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                            plane.competition_id.buffer(),
                            plane.competition_id.MAX_SIZE))
    changed = true;

  UpdateButtons();
}

static void
OnAircraftTypeClicked(gcc_unused WndButton &button)
{
  Plane &plane = CommonInterface::SetSettingsComputer().plane;

  if (dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                            plane.type.buffer(),
                            plane.type.MAX_SIZE))
    changed = true;

  UpdateButtons();
}

static void
OnAircraftRegClicked(gcc_unused WndButton &button)
{
  Plane &plane = CommonInterface::SetSettingsComputer().plane;

  if (dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                            plane.registration.buffer(),
                            plane.registration.MAX_SIZE))
    changed = true;

  UpdateButtons();
}

static void
OnPilotNameClicked(gcc_unused WndButton &button)
{
  StaticString<100> tmp;
  if (!Profile::Get(szProfilePilotName, tmp))
    tmp.clear();

  if (dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                            tmp.buffer(), tmp.MAX_SIZE)) {
    Profile::Set(szProfilePilotName, tmp);
    changed = true;
  }
  UpdateButtons();
}

static void
OnLoggerIDClicked(gcc_unused WndButton &button)
{
  StaticString<100> tmp;
  if (!Profile::Get(szProfileLoggerID, tmp))
    tmp.clear();

  if (dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                            tmp.buffer(), tmp.MAX_SIZE)) {
    Profile::Set(szProfileLoggerID, tmp);
    changed = true;
  }
  ReadAssetNumber();
  UpdateButtons();
}


void
LoggerConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  changed = false;

  buttonPilotName = ((WndButton *)wf->FindByName(_T("cmdPilotName")));
  assert(buttonPilotName != NULL);
  buttonPilotName->SetOnClickNotify(OnPilotNameClicked);

  buttonAircraftType = ((WndButton *)wf->FindByName(_T("cmdAircraftType")));
  assert(buttonAircraftType != NULL);
  buttonAircraftType->SetOnClickNotify(OnAircraftTypeClicked);

  buttonAircraftReg = ((WndButton *)wf->FindByName(_T("cmdAircraftReg")));
  assert(buttonAircraftReg != NULL);
  buttonAircraftReg->SetOnClickNotify(OnAircraftRegClicked);

  buttonCompetitionId = ((WndButton *)wf->FindByName(_T("cmdCompetitionId")));
  assert(buttonCompetitionId != NULL);
  buttonCompetitionId->SetOnClickNotify(OnCompetitionIdClicked);

  buttonLoggerID = ((WndButton *)wf->FindByName(_T("cmdLoggerID")));
  assert(buttonLoggerID != NULL);
  buttonLoggerID->SetOnClickNotify(OnLoggerIDClicked);

  UpdateButtons();

  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                   settings_computer.LoggerTimeStepCruise);

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                   settings_computer.LoggerTimeStepCircling);

  LoadFormProperty(*wf, _T("prpLoggerShortName"),
                   settings_computer.LoggerShortName);

  LoadFormProperty(*wf, _T("prpDisableAutoLogger"),
                   !settings_computer.DisableAutoLogger);
}


bool
LoggerConfigPanel::Save()
{
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  PlaneGlue::ToProfile(settings_computer.plane);

  changed |= SaveFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                              szProfileLoggerTimeStepCruise,
                              settings_computer.LoggerTimeStepCruise);

  changed |= SaveFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                              szProfileLoggerTimeStepCircling,
                              settings_computer.LoggerTimeStepCircling);

  changed |= SaveFormProperty(*wf, _T("prpLoggerShortName"),
                              szProfileLoggerShort,
                              settings_computer.LoggerShortName);

  /* GUI label is "Enable Auto Logger" */
  changed |= SaveFormPropertyNegated(*wf, _T("prpDisableAutoLogger"),
                                     szProfileDisableAutoLogger,
                                     settings_computer.DisableAutoLogger);

  return changed;
}
