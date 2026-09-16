// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightConfigPanel.hpp"

#ifdef HAVE_HTTP

#include "ConfigListPanel.hpp"
#include "DataGlobals.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/SkySight/Regions.hpp"
#include "Weather/SkySight/SkySightClient.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <string>
#include <vector>

/** The account and the region of SkySight. */
class SkySightConfigPanel final : public ConfigListPanel {
  StaticString<64> email, password;
  StaticString<32> region;

  /** One region which may be chosen. */
  struct Region {
    std::string id, name;
  };

  /** the regions of the client, or the built-in ones without it */
  std::vector<Region> regions;

private:
  void LoadRegions() noexcept;
  void PickRegion() noexcept;

  /** The name of the region which is chosen; its id if it is unknown. */
  [[gnu::pure]]
  const char *GetRegionName() const noexcept;

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
SkySightConfigPanel::LoadRegions() noexcept
{
  regions.clear();

  if (const auto skysight = DataGlobals::GetSkySight(); skysight != nullptr) {
    for (const auto &candidate : skysight->GetRegions())
      regions.push_back({candidate.id, gettext(candidate.name.c_str())});

    /* the client falls back to its own region for an unknown one */
    bool known = false;
    for (const auto &candidate : regions)
      known |= candidate.id == region.c_str();

    if (!known)
      region = skysight->GetRegion();
  } else {
    for (const auto &candidate : SKYSIGHT_REGIONS)
      regions.push_back({candidate.id, gettext(candidate.name)});

    region = FindSkySightRegionById(region.c_str()).id;
  }
}

const char *
SkySightConfigPanel::GetRegionName() const noexcept
{
  for (const auto &candidate : regions)
    if (candidate.id == region.c_str())
      return candidate.name.c_str();

  return region.c_str();
}

void
SkySightConfigPanel::PickRegion() noexcept
{
  std::vector<PickerChoice> choices;
  int current = -1;

  for (const auto &candidate : regions) {
    if (candidate.id == region.c_str())
      current = choices.size();

    choices.push_back({candidate.name.c_str()});
  }

  const int picked =
    PickChoice(C_("Setting", "SkySight Region"),
               _("Select the SkySight region used for live weather layers."),
               choices, current);
  if (picked < 0 || picked == current)
    return;

  region = regions[picked].id.c_str();
  Refresh();
}

void
SkySightConfigPanel::LoadSettings() noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  email = settings.skysight.email;
  password = settings.skysight.password;
  region = settings.skysight.region;

  LoadRegions();
}

void
SkySightConfigPanel::Fill() noexcept
{
  AddGroup();

  AddTextItem(C_("Setting", "SkySight Email"),
              _("The e-mail address you use to sign in to skysight.io."),
              email);

  AddTextItem(C_("Setting", "SkySight Password"),
              _("Your SkySight password."),
              password, true);

  AddGroup();

  AddItem(C_("Setting", "SkySight Region"), [this](){ PickRegion(); },
          {.value = GetRegionName(), .chevron = true});
}

bool
SkySightConfigPanel::Leave() noexcept
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
SkySightConfigPanel::Save(bool &_changed) noexcept
{
  auto &settings = CommonInterface::SetComputerSettings().weather;

  const bool email_changed =
    Profile::Update(ProfileKeys::SkySightEmail, settings.skysight.email,
                    email);
  const bool password_changed =
    Profile::Update(ProfileKeys::SkySightPassword,
                    settings.skysight.password, password);
  const bool region_changed =
    Profile::Update(ProfileKeys::SkySightRegion, settings.skysight.region,
                    region);

  const bool changed = email_changed || password_changed || region_changed;

  if (changed)
    if (auto skysight = DataGlobals::GetSkySight())
      skysight->Init();

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateSkySightConfigPanel()
{
  return std::make_unique<SkySightConfigPanel>();
}

#else

std::unique_ptr<Widget>
CreateSkySightConfigPanel()
{
  return {};
}

#endif
