// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "net/http/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

/** The weather sources which need nothing but a switch. */
class WeatherConfigPanel final : public ConfigListPanel {
#ifdef HAVE_HTTP
  bool enable_tim;
#endif

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
WeatherConfigPanel::LoadSettings() noexcept
{
#ifdef HAVE_HTTP
  enable_tim = CommonInterface::GetComputerSettings().weather.enable_tim;
#endif
}

void
WeatherConfigPanel::Fill() noexcept
{
#ifdef HAVE_HTTP
  AddGroup();

  AddToggleItem(_("Thermal Information Map"),
                _("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
                enable_tim);
#endif
}

bool
WeatherConfigPanel::Save(bool &_changed) noexcept
{
#ifdef HAVE_HTTP
  auto &settings = CommonInterface::SetComputerSettings().weather;

  _changed |= Profile::Update(ProfileKeys::EnableThermalInformationMap,
                              settings.enable_tim, enable_tim);
#else
  (void)_changed;
#endif

  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
