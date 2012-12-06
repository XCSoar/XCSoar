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

#include "LoggerInfoConfigPanel.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  PilotName,
  AircraftType,
  AircraftReg,
  CompetitionID,
  LoggerID,
};

class LoggerInfoConfigPanel : public RowFormWidget {
public:
  LoggerInfoConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
LoggerInfoConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const LoggerSettings &logger = settings_computer.logger;
  const Plane &plane = settings_computer.plane;

  RowFormWidget::Prepare(parent, rc);

  AddText(_("Pilot name"), NULL, logger.pilot_name);

  AddText(_("Aircraft type"), NULL, plane.type);
  AddText(_("Aircraft reg."), NULL, plane.registration);
  AddText(_("Competition ID"), NULL, plane.competition_id);

  AddText(_("Logger ID"), NULL, logger.logger_id);
}

bool
LoggerInfoConfigPanel::Save(bool &changed, bool &require_restart)
{
  bool plane_settings_changed = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  LoggerSettings &logger = settings_computer.logger;
  Plane &plane = settings_computer.plane;

  changed |= SaveValue(PilotName, ProfileKeys::PilotName,
                       logger.pilot_name.buffer(), logger.pilot_name.MAX_SIZE);

  plane_settings_changed |= SaveValue(AircraftType, plane.type.buffer(),
                                      plane.type.MAX_SIZE);
  plane_settings_changed |= SaveValue(AircraftReg, plane.registration.buffer(),
                                      plane.registration.MAX_SIZE);
  plane_settings_changed |= SaveValue(CompetitionID, plane.competition_id.buffer(),
                                      plane.competition_id.MAX_SIZE);
  changed |= plane_settings_changed;

  changed |= SaveValue(LoggerID, ProfileKeys::LoggerID,
                       logger.logger_id.buffer(), logger.logger_id.MAX_SIZE);

  if (plane_settings_changed) {
    PlaneGlue::DetachFromPlaneFile();
    PlaneGlue::ToProfile(settings_computer.plane);
  }

  return true;
}

Widget *
CreateLoggerInfoConfigPanel()
{
  return new LoggerInfoConfigPanel();
}
