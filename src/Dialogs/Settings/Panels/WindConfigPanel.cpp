// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

/**
 * Where the wind comes from: the estimates XCSoar makes itself and
 * the ones it takes from other instruments.
 */
class WindConfigPanel final : public ConfigListPanel {
  bool circling_wind, zig_zag_wind, external_wind;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
WindConfigPanel::LoadSettings() noexcept
{
  const WindSettings &settings = CommonInterface::GetComputerSettings().wind;

  circling_wind = settings.circling_wind;
  zig_zag_wind = settings.zig_zag_wind;
  external_wind = settings.external_wind;
}

void
WindConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem(_("Circling wind"),
                _("Estimate the wind vector while circling. Requires only a GPS."),
                circling_wind);

  AddToggleItem(_("ZigZag wind"),
                _("Estimate the wind vector during glides. Requires an airspeed sensor."),
                zig_zag_wind);

  AddToggleItem(_("External wind"),
                _("Should XCSoar accept wind estimates from other instruments?"),
                external_wind);
}

bool
WindConfigPanel::Save(bool &_changed) noexcept
{
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;

  bool changed = false;

  /* the two estimates share one key */
  if (settings.circling_wind != circling_wind ||
      settings.zig_zag_wind != zig_zag_wind) {
    settings.circling_wind = circling_wind;
    settings.zig_zag_wind = zig_zag_wind;
    Profile::Set(ProfileKeys::AutoWind, settings.GetLegacyAutoWindMode());
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::ExternalWind,
                             settings.external_wind, external_wind);

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateWindConfigPanel()
{
  return std::make_unique<WindConfigPanel>();
}
