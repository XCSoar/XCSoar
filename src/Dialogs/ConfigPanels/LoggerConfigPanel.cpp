/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "DataField/Enum.hpp"

enum ControlIndex {
  LoggerTimeStepCruise,
  LoggerTimeStepCircling,
  PilotName,
  AircraftType,
  AircraftReg,
  CompetitionID,
  LoggerID,
  LoggerShortName,
  DisableAutoLogger
};

class LoggerConfigPanel : public RowFormWidget {
public:
  LoggerConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

static const StaticEnumChoice auto_logger_list[] = {
  { (unsigned)LoggerSettings::AutoLogger::ON, N_("On"), NULL },
  { (unsigned)LoggerSettings::AutoLogger::START_ONLY, N_("Start only"), NULL },
  { (unsigned)LoggerSettings::AutoLogger::OFF, N_("Off"), NULL },
  { 0 }
};

void
LoggerConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const Plane &plane = settings_computer.plane;

  RowFormWidget::Prepare(parent, rc);

  // Expert item (TODO)
  AddInteger(_("Time step cruise"),
           _("This is the time interval between logged points when not circling."),
           _T("%u s"), _T("%u"), 1, 30, 1,
           settings_computer.logger_time_step_cruise);

  // Expert item
  AddInteger(_("Time step circling"),
          _("This is the time interval between logged points when circling."),
          _T("%u s"), _T("%u"),  1, 30, 1,
          settings_computer.logger_time_step_circling);

  TCHAR tmp_text[100];
  Profile::Get(szProfilePilotName, tmp_text, 100);
  AddText(_("Pilot name"), _T(""), tmp_text);

  AddText(_("Aircraft type"), _T(""), plane.type);
  AddText(_("Aircraft reg."), _T(""), plane.registration);
  AddText(_("Competition ID"), _T(""), plane.competition_id);

  Profile::Get(szProfileLoggerID, tmp_text, 100);
  AddText(_("Logger ID"), _T(""), tmp_text);

  // Expert item
  AddBoolean(_("Short file name"),
             _("This determines whether the logger uses the short IGC file name or the "
                 "long IGC file name. Example short name (81HXABC1.IGC), long name "
                 "(2008-01-18-XXX-ABC-01.IGC)."),
             settings_computer.logger_short_name);

  // Expert item
  AddEnum(_("Auto. logger"),
          _("Enables the automatic starting and stopping of logger on takeoff and landing "
            "respectively. Disable when flying paragliders."),
          auto_logger_list, (unsigned)settings_computer.auto_logger);
}

bool
LoggerConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  Plane &plane = settings_computer.plane;

  unsigned int tmp_value = settings_computer.logger_time_step_cruise;
  changed |= SaveValue(LoggerTimeStepCruise, szProfileLoggerTimeStepCruise, tmp_value);
  settings_computer.logger_time_step_cruise = tmp_value;

  tmp_value = settings_computer.logger_time_step_circling;
  changed |= SaveValue(LoggerTimeStepCircling, szProfileLoggerTimeStepCircling, tmp_value);
  settings_computer.logger_time_step_circling = tmp_value;

  TCHAR tmp_text[100];
  Profile::Get(szProfilePilotName, tmp_text, 100);
  changed |= SaveValue(PilotName, szProfilePilotName, tmp_text, 100);

  changed |= SaveValue(AircraftType, plane.type.buffer(), plane.type.MAX_SIZE);
  changed |= SaveValue(AircraftReg, plane.registration.buffer(), plane.registration.MAX_SIZE);
  changed |= SaveValue(CompetitionID, plane.competition_id.buffer(), plane.competition_id.MAX_SIZE);

  Profile::Get(szProfileLoggerID, tmp_text, 100);
  changed |= SaveValue(LoggerID, szProfileLoggerID, tmp_text, 100);

  changed |= SaveValue(LoggerShortName, szProfileLoggerShort,
                       settings_computer.logger_short_name);

  /* GUI label is "Enable Auto Logger" */
  changed |= SaveValueEnum(DisableAutoLogger, szProfileAutoLogger,
                           settings_computer.auto_logger);

  if (changed)
    PlaneGlue::ToProfile(settings_computer.plane);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateLoggerConfigPanel()
{
  return new LoggerConfigPanel();
}
