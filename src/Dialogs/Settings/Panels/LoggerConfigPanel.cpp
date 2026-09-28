// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LoggerConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Form/DataField/Enum.hpp"
#include "Logger/NMEALogger.hpp"
#include "UtilsSettings.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

using namespace std::chrono;

static constexpr StaticEnumChoice auto_logger_list[] = {
  { LoggerSettings::AutoLogger::ON, N_("On") },
  { LoggerSettings::AutoLogger::START_ONLY, N_("Start only") },
  { LoggerSettings::AutoLogger::OFF, N_("Off") },
  nullptr
};

/**
 * The IGC flight log: who flies, how often a point is logged and
 * when the logger starts.
 */
class LoggerConfigPanel final : public ConfigListPanel {
  StaticString<64> pilot_name, copilot_name;
  StaticString<32> logger_id;

  /** in kg */
  unsigned crew_mass_template;

  duration<unsigned> time_step_cruise, time_step_circling;
  LoggerSettings::AutoLogger auto_logger;
  bool enable_nmea_logger, enable_flight_logger;

private:
  void AddCrewMassItem() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
LoggerConfigPanel::LoadSettings() noexcept
{
  const LoggerSettings &logger =
    CommonInterface::GetComputerSettings().logger;

  pilot_name = logger.pilot_name;
  copilot_name = logger.copilot_name;
  logger_id = logger.logger_id;
  crew_mass_template = logger.crew_mass_template;
  time_step_cruise = logger.time_step_cruise;
  time_step_circling = logger.time_step_circling;
  auto_logger = logger.auto_logger;
  enable_nmea_logger = logger.enable_nmea_logger;
  enable_flight_logger = logger.enable_flight_logger;
}

void
LoggerConfigPanel::AddCrewMassItem() noexcept
{
  const char *caption = _("Crew weight default");
  const char *help =
    _("Default for all weight loaded to the glider beyond the empty weight and besides "
      "the water ballast.");

  StaticString<32> mass;
  FormatUserMass(crew_mass_template, mass.buffer());

  /* one choice per 5 units of the user, up to 300 kg */
  AddItem(caption, [this, caption, help](){
    int user_value = iround(Units::ToUserMass(crew_mass_template));
    if (PickNumber(caption, help,
                   0, iround(Units::ToUserMass(300)), 5, user_value,
                   [](StaticString<32> &s, int v){
                     FormatUserMass(Units::ToSysMass(v), s.buffer());
                   })) {
      crew_mass_template = iround(Units::ToSysMass(user_value));
      Refresh();
    }
  }, {.value = mass.c_str(), .chevron = true});
}

void
LoggerConfigPanel::Fill() noexcept
{
  AddGroup();

  AddTextItem(_("Pilot name"),
              _("Name of the pilot in command, recorded in the IGC flight log."),
              pilot_name);

  AddTextItem(_("CoPilot name"),
              _("The co-pilot name recorded in the IGC flight log."),
              copilot_name);

  AddCrewMassItem();

  if (!IsExpert())
    return;

  /* when and how often the logger writes */
  AddGroup();

  AddDurationItem(_("Time step cruise"),
                  _("This is the time interval between logged points when not circling."),
                  1, 30, 1, time_step_cruise);

  AddDurationItem(_("Time step circling"),
                  _("This is the time interval between logged points when circling."),
                  1, 30, 1, time_step_circling);

  AddEnumItem(_("Auto. logger"),
              _("Enables the automatic starting and stopping of logger on takeoff and landing "
                "respectively. Disable when flying paragliders."),
              auto_logger_list, auto_logger);

  AddGroup();

  AddToggleItem(_("NMEA Logger"),
                _("Enable the NMEA logger on startup? If this option is disabled, "
                  "the NMEA logger can still be started manually."),
                enable_nmea_logger);

  AddToggleItem(_("Log book"), _("Logs each start and landing."),
                enable_flight_logger);

  AddTextItem(_("Logger ID"),
              _("The three-letter logger ID used in the IGC filename."),
              logger_id);
}

bool
LoggerConfigPanel::Save(bool &changed) noexcept
{
  LoggerSettings &logger = CommonInterface::SetComputerSettings().logger;

  changed |= Profile::Update(ProfileKeys::PilotName, logger.pilot_name,
                             pilot_name);
  changed |= Profile::Update(ProfileKeys::CoPilotName, logger.copilot_name,
                             copilot_name);
  changed |= Profile::Update(ProfileKeys::CrewWeightTemplate,
                             logger.crew_mass_template, crew_mass_template);
  changed |= Profile::Update(ProfileKeys::LoggerTimeStepCruise,
                             logger.time_step_cruise, time_step_cruise);
  changed |= Profile::Update(ProfileKeys::LoggerTimeStepCircling,
                             logger.time_step_circling, time_step_circling);
  changed |= Profile::Update(ProfileKeys::AutoLogger, logger.auto_logger,
                             auto_logger);
  changed |= Profile::Update(ProfileKeys::EnableNMEALogger,
                             logger.enable_nmea_logger, enable_nmea_logger);

  if (logger.enable_nmea_logger && backend_components->nmea_logger != nullptr)
    backend_components->nmea_logger->Enable();

  if (Profile::Update(ProfileKeys::EnableFlightLogger,
                      logger.enable_flight_logger, enable_flight_logger)) {
    changed = true;

    /* currently, the GlueFlightLogger instance is created on startup
       only, which means XCSoar needs to be restarted to apply the new
       setting */
    require_restart = true;
  }

  changed |= Profile::Update(ProfileKeys::LoggerID, logger.logger_id,
                             logger_id);

  return true;
}

std::unique_ptr<Widget>
CreateLoggerConfigPanel()
{
  return std::make_unique<LoggerConfigPanel>();
}
