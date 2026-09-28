// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "net/State.hpp"
#include "Interface.hpp"
#include "util/NumberParser.hpp"

#include <climits>
#include <cstdlib>

#if (defined HAVE_SKYLINES_TRACKING || defined HAVE_LIVETRACK24)

static constexpr StaticEnumChoice tracking_intervals[] = {
  { 1, "1 sec" },
  { 2, "2 sec" },
  { 3, "3 sec" },
  { 5, "5 sec" },
  { 10, "10 sec" },
  { 15, "15 sec" },
  { 20, "20 sec" },
  { 30, "30 sec" },
  { 45, "45 sec" },
  { 60, "1 min" },
  { 120, "2 min" },
  { 180, "3 min" },
  { 300, "5 min" },
  { 600, "10 min" },
  { 900, "15 min" },
  { 1200, "20 min" },
  { 1800, "30 min" },
  { 2400, "40 min" },
  { 3000, "50 min" },
  { 3600, "60 min" },
  nullptr,
};

/** The interval of the list which is nearest to @p value. */
[[gnu::pure]]
static unsigned
FindClosestTrackingInterval(unsigned value) noexcept
{
  unsigned closest_value = 0;
  int closest_diff = INT_MAX;

  for (const StaticEnumChoice *p = tracking_intervals;
       p->display_string != nullptr; ++p) {
    const int diff = std::abs(int(value) - int(p->id));
    if (diff < closest_diff) {
      closest_diff = diff;
      closest_value = p->id;
    }
  }

  return closest_value;
}

#endif

#ifdef HAVE_LIVETRACK24

static constexpr const char *server_list[] = {
  "www.livetrack24.com",
  "test.livetrack24.com",
  "livexc.dhv.de",
};

static constexpr StaticEnumChoice vehicle_type_list[] = {
  { LiveTrack24::Settings::VehicleType::GLIDER, N_("Glider") },
  { LiveTrack24::Settings::VehicleType::PARAGLIDER, N_("Paraglider") },
  { LiveTrack24::Settings::VehicleType::POWERED_AIRCRAFT, N_("Powered aircraft") },
  { LiveTrack24::Settings::VehicleType::HOT_AIR_BALLOON, N_("Hot-air balloon") },
  { LiveTrack24::Settings::VehicleType::HANGGLIDER_FLEX, N_("Hangglider (Flex/FAI1)") },
  { LiveTrack24::Settings::VehicleType::HANGGLIDER_RIGID, N_("Hangglider (Rigid/FAI5)") },
  nullptr,
};

#endif

/** The live tracking services: SkyLines and LiveTrack24. */
class TrackingConfigPanel final : public ConfigListPanel {
#ifdef HAVE_SKYLINES_TRACKING
  bool sl_enabled;
#ifdef HAVE_NET_STATE_ROAMING
  bool sl_roaming;
#endif
  unsigned sl_interval;
  bool sl_traffic_enabled, sl_near_traffic_enabled;

  /** the key in hexadecimal, as the user types it */
  StaticString<64> sl_key;
#endif

#ifdef HAVE_LIVETRACK24
  bool lt24_enabled;
  unsigned lt24_interval;
  LiveTrack24::Settings::VehicleType lt24_vehicle_type;
  StaticString<64> lt24_vehicle_name, lt24_server, lt24_username,
    lt24_password;
#endif

private:
#ifdef HAVE_SKYLINES_TRACKING
  void AddSkyLinesItems() noexcept;
#endif
#ifdef HAVE_LIVETRACK24
  void AddLiveTrack24Items() noexcept;
  void PickServer() noexcept;
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
TrackingConfigPanel::LoadSettings() noexcept
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

#ifdef HAVE_SKYLINES_TRACKING
  sl_enabled = settings.skylines.enabled;
#ifdef HAVE_NET_STATE_ROAMING
  sl_roaming = settings.skylines.roaming;
#endif
  sl_interval = FindClosestTrackingInterval(settings.skylines.interval);
  sl_traffic_enabled = settings.skylines.traffic_enabled;
  sl_near_traffic_enabled = settings.skylines.near_traffic_enabled;

  if (settings.skylines.key != 0)
    sl_key.UnsafeFormat("%llX", (unsigned long long)settings.skylines.key);
  else
    sl_key.clear();
#endif

#ifdef HAVE_LIVETRACK24
  lt24_enabled = settings.livetrack24.enabled;
  lt24_interval = FindClosestTrackingInterval(settings.livetrack24.interval);
  lt24_vehicle_type = settings.livetrack24.vehicleType;
  lt24_vehicle_name = settings.livetrack24.vehicle_name;
  lt24_server = settings.livetrack24.server;
  lt24_username = settings.livetrack24.username;
  lt24_password = settings.livetrack24.password;
#endif
}

#ifdef HAVE_SKYLINES_TRACKING

void
TrackingConfigPanel::AddSkyLinesItems() noexcept
{
  AddGroup("SkyLines");

  AddToggleItem("SkyLines",
                _("Enable live tracking via the SkyLines server "
                  "(tracking.skylines.aero)."),
                sl_enabled);

#ifdef HAVE_NET_STATE_ROAMING
  AddToggleItem(_("Roaming"),
                _("Allow tracking when on a roaming mobile data connection."),
                sl_roaming, nullptr, !sl_enabled);
#endif

  /* the items below are greyed out while the service is off */
  if (sl_enabled)
    AddEnumItem(_("Tracking Interval"), nullptr, tracking_intervals,
                sl_interval);
  else
    AddItem(_("Tracking Interval"),
            {.value = GetEnumCaption(tracking_intervals, sl_interval),
             .disabled = true});

  AddToggleItem(_("Track friends"),
                _("Download the position of your SkyLines friends live from "
                  "the SkyLines server."),
                sl_traffic_enabled, nullptr, !sl_enabled);

  AddToggleItem(_("Show nearby traffic"),
                _("Download the position of nearby SkyLines users live from "
                  "the SkyLines server."),
                sl_near_traffic_enabled, nullptr,
                !sl_enabled || !sl_traffic_enabled);

  if (sl_enabled)
    AddTextItem("Key",
                _("Your SkyLines tracking key. "
                  "This is used to identify your aircraft on the server."),
                sl_key);
  else
    AddItem("Key", {.value = sl_key.c_str(), .disabled = true});
}

#endif

#ifdef HAVE_LIVETRACK24

void
TrackingConfigPanel::PickServer() noexcept
{
  PickerChoice choices[std::size(server_list)];
  int current = -1;

  for (unsigned i = 0; i < std::size(server_list); ++i) {
    choices[i] = {server_list[i]};
    if (lt24_server == server_list[i])
      current = i;
  }

  const int picked = PickChoice(_("Server"), nullptr, choices, current);
  if (picked < 0 || picked == current)
    return;

  lt24_server = server_list[picked];
  Refresh();
}

void
TrackingConfigPanel::AddLiveTrack24Items() noexcept
{
  AddGroup("LiveTrack24");

  AddToggleItem("LiveTrack24", nullptr, lt24_enabled);

  if (!lt24_enabled) {
    /* the settings of a service which is off are read only */
    AddItem(_("Tracking Interval"),
            {.value = GetEnumCaption(tracking_intervals, lt24_interval),
             .disabled = true});
    AddItem(_("Vehicle Type"),
            {.value = GetEnumCaption(vehicle_type_list,
                                     (unsigned)lt24_vehicle_type),
             .disabled = true});
    AddItem(_("Vehicle Name"),
            {.value = lt24_vehicle_name.c_str(), .disabled = true});
    AddItem(_("Server"), {.value = lt24_server.c_str(), .disabled = true});
    AddItem(_("Username"),
            {.value = lt24_username.c_str(), .disabled = true});
    AddItem(_("Password"), {.disabled = true});
    return;
  }

  AddEnumItem(_("Tracking Interval"), nullptr, tracking_intervals,
              lt24_interval);

  AddEnumItem(_("Vehicle Type"), _("Type of vehicle used."),
              vehicle_type_list, lt24_vehicle_type);

  AddTextItem(_("Vehicle Name"), _("Name of vehicle used."),
              lt24_vehicle_name);

  AddItem(_("Server"), [this](){ PickServer(); },
          {.value = lt24_server.c_str(), .chevron = true});

  AddTextItem(_("Username"), nullptr, lt24_username);
  AddTextItem(_("Password"), nullptr, lt24_password, true);
}

#endif

void
TrackingConfigPanel::Fill() noexcept
{
#ifdef HAVE_SKYLINES_TRACKING
  AddSkyLinesItems();
#endif

#ifdef HAVE_LIVETRACK24
  AddLiveTrack24Items();
#endif
}

bool
TrackingConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

#ifdef HAVE_LIVETRACK24
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingInterval,
                             settings.livetrack24.interval, lt24_interval);
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingVehicleType,
                             settings.livetrack24.vehicleType,
                             lt24_vehicle_type);
  changed |= Profile::Update(ProfileKeys::LiveTrack24TrackingVehicleName,
                             settings.livetrack24.vehicle_name,
                             lt24_vehicle_name);
#endif

#ifdef HAVE_SKYLINES_TRACKING
  changed |= Profile::Update(ProfileKeys::SkyLinesTrackingEnabled,
                             settings.skylines.enabled, sl_enabled);
#ifdef HAVE_NET_STATE_ROAMING
  changed |= Profile::Update(ProfileKeys::SkyLinesRoaming,
                             settings.skylines.roaming, sl_roaming);
#endif
  changed |= Profile::Update(ProfileKeys::SkyLinesTrackingInterval,
                             settings.skylines.interval, sl_interval);
  changed |= Profile::Update(ProfileKeys::SkyLinesTrafficEnabled,
                             settings.skylines.traffic_enabled,
                             sl_traffic_enabled);
  changed |= Profile::Update(ProfileKeys::SkyLinesNearTrafficEnabled,
                             settings.skylines.near_traffic_enabled,
                             sl_near_traffic_enabled);

  /* the key is stored as the user typed it */
  if (const uint64_t key = ParseUint64(sl_key.c_str(), nullptr, 16);
      key != settings.skylines.key) {
    settings.skylines.key = key;
    Profile::Set(ProfileKeys::SkyLinesTrackingKey, sl_key.c_str());
    changed = true;
  }
#endif

#ifdef HAVE_LIVETRACK24
  changed |= Profile::Update(ProfileKeys::LiveTrack24Enabled,
                             settings.livetrack24.enabled, lt24_enabled);
  changed |= Profile::Update(ProfileKeys::LiveTrack24Server,
                             settings.livetrack24.server, lt24_server);
  changed |= Profile::Update(ProfileKeys::LiveTrack24Username,
                             settings.livetrack24.username, lt24_username);
  changed |= Profile::Update(ProfileKeys::LiveTrack24Password,
                             settings.livetrack24.password, lt24_password);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTrackingConfigPanel()
{
  return std::make_unique<TrackingConfigPanel>();
}
