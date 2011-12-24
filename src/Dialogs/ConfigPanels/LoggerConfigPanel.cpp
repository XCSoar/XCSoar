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
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

class LoggerConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
LoggerConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
LoggerConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
LoggerConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_LOGGERCONFIGPANEL") :
                               _T("IDR_XML_LOGGERCONFIGPANEL_L"));

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const Plane &plane = settings_computer.plane;

  LoadFormProperty(form, _T("prpLoggerTimeStepCruise"),
                   settings_computer.logger_time_step_cruise);

  LoadFormProperty(form, _T("prpLoggerTimeStepCircling"),
                   settings_computer.logger_time_step_circling);

  LoadFormPropertyFromProfile(form, _T("PilotName"), szProfilePilotName);
  LoadFormProperty(form, _T("AircraftType"), plane.type);
  LoadFormProperty(form, _T("AircraftReg"), plane.registration);
  LoadFormProperty(form, _T("CompetitionID"), plane.competition_id);
  LoadFormPropertyFromProfile(form, _T("LoggerID"), szProfileLoggerID);

  LoadFormProperty(form, _T("prpLoggerShortName"),
                   settings_computer.logger_short_name);

  LoadFormProperty(form, _T("prpDisableAutoLogger"),
                   !settings_computer.auto_logger_disabled);
}

bool
LoggerConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer =
    XCSoarInterface::SetComputerSettings();
  Plane &plane = settings_computer.plane;

  changed |= SaveFormProperty(form, _T("prpLoggerTimeStepCruise"),
                              szProfileLoggerTimeStepCruise,
                              settings_computer.logger_time_step_cruise);

  changed |= SaveFormProperty(form, _T("prpLoggerTimeStepCircling"),
                              szProfileLoggerTimeStepCircling,
                              settings_computer.logger_time_step_circling);

  changed |= SaveFormPropertyToProfile(form, _T("PilotName"),
                                       szProfilePilotName);
  changed |= SaveFormProperty(form, _T("AircraftType"), plane.type);
  changed |= SaveFormProperty(form, _T("AircraftReg"), plane.registration);
  changed |= SaveFormProperty(form, _T("CompetitionID"), plane.competition_id);
  changed |= SaveFormPropertyToProfile(form, _T("LoggerID"), szProfileLoggerID);

  changed |= SaveFormProperty(form, _T("prpLoggerShortName"),
                              szProfileLoggerShort,
                              settings_computer.logger_short_name);

  /* GUI label is "Enable Auto Logger" */
  changed |= SaveFormPropertyNegated(form, _T("prpDisableAutoLogger"),
                                     szProfileDisableAutoLogger,
                                     settings_computer.auto_logger_disabled);

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
