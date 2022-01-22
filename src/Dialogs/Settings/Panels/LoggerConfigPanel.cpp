/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Logger/NMEALogger.hpp"
#include "UtilsSettings.hpp"
#include "Components.hpp"
#include "Units/Group.hpp"

using namespace std::chrono;

enum ControlIndex {
  PilotName,
  CoPilotName,
  CrewWeightTemplate,
  LoggerTimeStepCruise,
  LoggerTimeStepCircling,
  DisableAutoLogger,
  EnableNMEALogger,
  EnableFlightLogger,
  LoggerID,
};

class LoggerConfigPanel final : public RowFormWidget {
public:
  LoggerConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

static constexpr StaticEnumChoice auto_logger_list[] = {
  { (unsigned)LoggerSettings::AutoLogger::ON, N_("On"), nullptr },
  { (unsigned)LoggerSettings::AutoLogger::START_ONLY, N_("Start only"), nullptr },
  { (unsigned)LoggerSettings::AutoLogger::OFF, N_("Off"), nullptr },
  { 0 }
};

void
LoggerConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const LoggerSettings &logger = settings_computer.logger;

  RowFormWidget::Prepare(parent, rc);
  AddText(_("Pilot name"), nullptr, logger.pilot_name);

  AddText(_("CoPilot name"), nullptr, logger.copilot_name);

  AddFloat(_("Crew weight default"),
            _("Default for all weight loaded to the glider beyond the empty weight and besides "
                "the water ballast."),
            _T("%.0f %s"), _T("%.0f"),
            0, 300, 5, false, UnitGroup::MASS,
            logger.crew_mass_template);

  AddDuration(_("Time step cruise"),
              _("This is the time interval between logged points when not circling."),
              seconds{1}, seconds{30}, seconds{1}, logger.time_step_cruise);
  SetExpertRow(LoggerTimeStepCruise);

  AddDuration(_("Time step circling"),
              _("This is the time interval between logged points when circling."),
              seconds{1}, seconds{30}, seconds{1}, logger.time_step_circling);
  SetExpertRow(LoggerTimeStepCircling);

  AddEnum(_("Auto. logger"),
          _("Enables the automatic starting and stopping of logger on takeoff and landing "
            "respectively. Disable when flying paragliders."),
          auto_logger_list, (unsigned)logger.auto_logger);
  SetExpertRow(DisableAutoLogger);

  AddBoolean(_("NMEA logger"),
             _("Enable the NMEA logger on startup? If this option is disabled, "
                 "the NMEA logger can still be started manually."),
             logger.enable_nmea_logger);
  SetExpertRow(EnableNMEALogger);

  AddBoolean(_("Log book"), _("Logs each start and landing."),
             logger.enable_flight_logger);
  SetExpertRow(EnableFlightLogger);

  AddText(_("Logger ID"), nullptr, logger.logger_id);
  SetExpertRow(LoggerID);
}

bool
LoggerConfigPanel::Save(bool &changed) noexcept
{
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  LoggerSettings &logger = settings_computer.logger;

  changed |= SaveValue(PilotName, ProfileKeys::PilotName,
                       logger.pilot_name);

  changed |= SaveValue(CoPilotName, ProfileKeys::CoPilotName,
                       logger.copilot_name);

  changed |= SaveValue(CrewWeightTemplate, UnitGroup::MASS, ProfileKeys::CrewWeightTemplate,
                       logger.crew_mass_template);

  changed |= SaveValue(LoggerTimeStepCruise, ProfileKeys::LoggerTimeStepCruise,
                       logger.time_step_cruise);

  changed |= SaveValue(LoggerTimeStepCircling, ProfileKeys::LoggerTimeStepCircling,
                       logger.time_step_circling);

  /* GUI label is "Enable Auto Logger" */
  changed |= SaveValueEnum(DisableAutoLogger, ProfileKeys::AutoLogger,
                           logger.auto_logger);

  changed |= SaveValue(EnableNMEALogger, ProfileKeys::EnableNMEALogger,
                       logger.enable_nmea_logger);

  if (logger.enable_nmea_logger && nmea_logger != nullptr)
    nmea_logger->Enable();

  if (SaveValue(EnableFlightLogger, ProfileKeys::EnableFlightLogger,
                logger.enable_flight_logger)) {
    changed = true;

    /* currently, the GlueFlightLogger instance is created on startup
       only, which means XCSoar needs to be restarted to apply the new
       setting */
    require_restart = true;
  }

  changed |= SaveValue(LoggerID, ProfileKeys::LoggerID, logger.logger_id);

  return true;
}

std::unique_ptr<Widget>
CreateLoggerConfigPanel()
{
  return std::make_unique<LoggerConfigPanel>();
}
