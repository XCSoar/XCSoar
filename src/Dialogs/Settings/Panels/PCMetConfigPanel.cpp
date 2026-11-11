// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

/** The account of flugwetter.de. */
class PCMetConfigPanel final : public ConfigListPanel {
#ifdef HAVE_PCMET
  StaticString<64> username, password;
  bool mosmix_forecast_temperature = false;

  /** the forecast switch belongs to the configuration dialogue only */
  const bool with_forecast_switch;
#endif

public:
#ifdef HAVE_PCMET
  explicit PCMetConfigPanel(bool _with_forecast_switch) noexcept
    :with_forecast_switch(_with_forecast_switch) {}
#else
  explicit PCMetConfigPanel(bool) noexcept {}
#endif

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Leave() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
PCMetConfigPanel::LoadSettings() noexcept
{
#ifdef HAVE_PCMET
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  username = settings.pcmet.www_credentials.username;
  password = settings.pcmet.www_credentials.password;
  mosmix_forecast_temperature = settings.mosmix_forecast_temperature;
#endif
}

void
PCMetConfigPanel::Fill() noexcept
{
#ifdef HAVE_PCMET
  AddGroup();

  AddTextItem(_("pc_met Username"), nullptr, username);
  AddTextItem(_("pc_met Password"), nullptr, password, true);

  /* No account of its own: MOSMIX is open data.  It sits here because
     it is the DWD, which is what a pilot looking for it will read. */
  if (with_forecast_switch)
    AddToggleItem(_("Max. temp. from forecast"),
                  _("Fill Max. temp. in the flight setup from the day's "
                    "DWD forecast for the nearest MOSMIX station.  Needs "
                    "a position fix, and fetches once a day; no account."),
                  mosmix_forecast_temperature);
#endif
}

bool
PCMetConfigPanel::Leave() noexcept
{
  /* the list of the Weather group says whether the account is set
     up: store it as soon as the page is left, not only when the
     configuration is closed */
  bool changed = false;
  Save(changed);
  if (changed)
    Profile::Save();

  return ConfigListPanel::Leave();
}

bool
PCMetConfigPanel::Save(bool &_changed) noexcept
{
#ifdef HAVE_PCMET
  auto &settings = CommonInterface::SetComputerSettings().weather;

  const bool username_changed =
    Profile::Update(ProfileKeys::PCMetUsername,
                    settings.pcmet.www_credentials.username, username);
  const bool password_changed =
    Profile::Update(ProfileKeys::PCMetPassword,
                    settings.pcmet.www_credentials.password, password);

  bool changed = username_changed || password_changed;

  if (with_forecast_switch)
    changed |= Profile::Update(ProfileKeys::MosmixForecastTemperature,
                               settings.mosmix_forecast_temperature,
                               mosmix_forecast_temperature);

  _changed |= changed;
#else
  (void)_changed;
#endif

  return true;
}

std::unique_ptr<Widget>
CreatePCMetConfigPanel()
{
  return std::make_unique<PCMetConfigPanel>(true);
}

std::unique_ptr<Widget>
CreatePCMetCredentialsPanel()
{
  return std::make_unique<PCMetConfigPanel>(false);
}
