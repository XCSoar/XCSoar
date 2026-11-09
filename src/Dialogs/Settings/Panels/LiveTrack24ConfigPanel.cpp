// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LiveTrack24ConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "TrackingIntervalChoices.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Tracking/Features.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Interface.hpp"
#include "util/StaticString.hxx"
#include "util/StringAPI.hxx"

#ifdef HAVE_LIVETRACK24

static constexpr StaticEnumChoice server_list[] = {
  { 0, "www.livetrack24.com" },
  { 1, "test.livetrack24.com" },
  { 2, "livexc.dhv.de" },
  nullptr,
};

static constexpr StaticEnumChoice vehicle_type_list[] = {
  { LiveTrack24::Settings::VehicleType::GLIDER, N_("Glider") },
  { LiveTrack24::Settings::VehicleType::PARAGLIDER, N_("Paraglider") },
  { LiveTrack24::Settings::VehicleType::POWERED_AIRCRAFT,
    N_("Powered aircraft") },
  { LiveTrack24::Settings::VehicleType::HOT_AIR_BALLOON,
    N_("Hot-air balloon") },
  { LiveTrack24::Settings::VehicleType::HANGGLIDER_FLEX,
    N_("Hangglider (Flex/FAI1)") },
  { LiveTrack24::Settings::VehicleType::HANGGLIDER_RIGID,
    N_("Hangglider (Rigid/FAI5)") },
  nullptr,
};

/** LiveTrack24: whether to track, and which account to use. */
class LiveTrack24ConfigPanel final : public ConfigListPanel {
  bool enabled;
  unsigned interval, server;
  LiveTrack24::Settings::VehicleType vehicle_type;
  StaticString<64> vehicle_name, username, password;

  unsigned FindServer(const char *name) const noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

unsigned
LiveTrack24ConfigPanel::FindServer(const char *name) const noexcept
{
  for (const auto *i = server_list; i->display_string != nullptr; ++i)
    if (StringIsEqual(name, i->display_string))
      return i->id;

  return 0;
}

void
LiveTrack24ConfigPanel::LoadSettings() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.livetrack24;

  enabled = settings.enabled;
  interval = ClosestTrackingInterval(settings.interval);
  vehicle_type = settings.vehicleType;
  vehicle_name = settings.vehicle_name;
  server = FindServer(settings.server);
  username = settings.username;
  password = settings.password;
}

void
LiveTrack24ConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem(_("Enable"), nullptr, enabled);

  StaticString<16> interval_text;
  FormatTrackingInterval(interval_text, interval);
  AddItem(_("Tracking Interval"), [this](){
    if (PickTrackingInterval(interval))
      Refresh();
  },
          {.value = interval_text.c_str(), .chevron = enabled,
           .disabled = !enabled});

  const bool off = !enabled;
  AddEnumItem(_("Vehicle Type"), _("Type of vehicle used."),
              vehicle_type_list, vehicle_type, off);
  AddTextItem(_("Vehicle Name"), _("Name of vehicle used."),
              vehicle_name, false, off);
  AddEnumItem(_("Server"), nullptr, server_list, server, off);
  AddTextItem(_("Username"), nullptr, username, false, off);
  AddTextItem(_("Password"), nullptr, password, true, off);
}

bool
LiveTrack24ConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.livetrack24;

  changed |= Profile::Update(ProfileKeys::LiveTrack24Enabled,
                             settings.enabled, enabled);
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingInterval,
                             settings.interval, interval);
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingVehicleType,
                             settings.vehicleType, vehicle_type);
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingVehicleName,
                             settings.vehicle_name, vehicle_name);

  StaticString<64> server_text;
  server_text = GetEnumCaption(server_list, server);
  changed |= Profile::Update(ProfileKeys::LiveTrack24Server,
                             settings.server, server_text);

  changed |= Profile::Update(ProfileKeys::LiveTrack24Username,
                             settings.username, username);
  changed |= Profile::Update(ProfileKeys::LiveTrack24Password,
                             settings.password, password);

  _changed |= changed;
  return true;
}

#endif /* HAVE_LIVETRACK24 */

std::unique_ptr<Widget>
CreateLiveTrack24ConfigPanel()
{
#ifdef HAVE_LIVETRACK24
  return std::make_unique<LiveTrack24ConfigPanel>();
#else
  return nullptr;
#endif
}
