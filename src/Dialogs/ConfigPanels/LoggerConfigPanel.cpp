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

#include "LoggerConfigPanel.hpp"
#include "Form/Util.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Plane/PlaneGlue.hpp"

static WndForm* wf = NULL;

void
LoggerConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();
  const Plane &plane = settings_computer.plane;

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                   settings_computer.LoggerTimeStepCruise);

  LoadFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                   settings_computer.LoggerTimeStepCircling);

  LoadFormPropertyFromProfile(*wf, _T("PilotName"), szProfilePilotName);
  LoadFormProperty(*wf, _T("AircraftType"), plane.type);
  LoadFormProperty(*wf, _T("AircraftReg"), plane.registration);
  LoadFormProperty(*wf, _T("CompetitionID"), plane.competition_id);
  LoadFormPropertyFromProfile(*wf, _T("LoggerID"), szProfileLoggerID);

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
  Plane &plane = settings_computer.plane;

  bool changed = false;

  changed |= SaveFormProperty(*wf, _T("prpLoggerTimeStepCruise"),
                              szProfileLoggerTimeStepCruise,
                              settings_computer.LoggerTimeStepCruise);

  changed |= SaveFormProperty(*wf, _T("prpLoggerTimeStepCircling"),
                              szProfileLoggerTimeStepCircling,
                              settings_computer.LoggerTimeStepCircling);

  changed |= SaveFormPropertyToProfile(*wf, _T("PilotName"),
                                       szProfilePilotName);
  changed |= SaveFormProperty(*wf, _T("AircraftType"), plane.type);
  changed |= SaveFormProperty(*wf, _T("AircraftReg"), plane.registration);
  changed |= SaveFormProperty(*wf, _T("CompetitionID"), plane.competition_id);
  changed |= SaveFormPropertyToProfile(*wf, _T("LoggerID"), szProfileLoggerID);

  changed |= SaveFormProperty(*wf, _T("prpLoggerShortName"),
                              szProfileLoggerShort,
                              settings_computer.LoggerShortName);

  /* GUI label is "Enable Auto Logger" */
  changed |= SaveFormPropertyNegated(*wf, _T("prpDisableAutoLogger"),
                                     szProfileDisableAutoLogger,
                                     settings_computer.DisableAutoLogger);

  if (changed)
    PlaneGlue::ToProfile(settings_computer.plane);

  return changed;
}
