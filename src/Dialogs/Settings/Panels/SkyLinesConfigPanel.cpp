// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkyLinesConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "TrackingIntervalChoices.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Tracking/Features.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "net/State.hpp"
#include "Interface.hpp"
#include "util/NumberParser.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_SKYLINES_TRACKING

/** SkyLines: whether to track, how often, and which traffic to show. */
class SkyLinesConfigPanel final : public ConfigListPanel {
  bool enabled;
#ifdef HAVE_NET_STATE_ROAMING
  bool roaming;
#endif
  bool traffic_enabled, near_traffic_enabled;
  unsigned interval;
  StaticString<64> key_text;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
SkyLinesConfigPanel::LoadSettings() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.skylines;

  enabled = settings.enabled;
#ifdef HAVE_NET_STATE_ROAMING
  roaming = settings.roaming;
#endif
  traffic_enabled = settings.traffic_enabled;
  near_traffic_enabled = settings.near_traffic_enabled;
  interval = ClosestTrackingInterval(settings.interval);

  if (settings.key != 0)
    key_text.Format("%llX", (unsigned long long)settings.key);
  else
    key_text.clear();
}

void
SkyLinesConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem(_("Enable"),
                _("Enable live tracking via the SkyLines server "
                  "(tracking.skylines.aero)."),
                enabled);

#ifdef HAVE_NET_STATE_ROAMING
  AddToggleItem(_("Roaming"),
                _("Allow tracking when on a roaming mobile data "
                  "connection."),
                roaming, nullptr, !enabled);
#endif

  StaticString<16> interval_text;
  FormatTrackingInterval(interval_text, interval);
  AddItem(_("Tracking Interval"), [this](){
    if (PickTrackingInterval(interval))
      Refresh();
  },
          {.value = interval_text.c_str(), .chevron = enabled,
           .disabled = !enabled});

  AddToggleItem(_("Track friends"),
                _("Download the position of your SkyLines friends "
                  "live from the SkyLines server."),
                traffic_enabled, nullptr, !enabled);

  AddToggleItem(_("Show nearby traffic"),
                _("Download the position of nearby SkyLines users "
                  "live from the SkyLines server."),
                near_traffic_enabled, nullptr,
                !enabled || !traffic_enabled);

  AddTextItem(C_("Setting", "Key"),
              _("Your SkyLines tracking key. "
                "This is used to identify your aircraft on the server."),
              key_text, false, !enabled);
}

bool
SkyLinesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines;

  changed |= Profile::Update(ProfileKeys::SkyLinesTrackingEnabled,
                             settings.enabled, enabled);

#ifdef HAVE_NET_STATE_ROAMING
  changed |= Profile::Update(ProfileKeys::SkyLinesRoaming,
                             settings.roaming, roaming);
#endif

  changed |= Profile::Update(ProfileKeys::SkyLinesTrackingInterval,
                             settings.interval, interval);

  changed |= Profile::Update(ProfileKeys::SkyLinesTrafficEnabled,
                             settings.traffic_enabled, traffic_enabled);
  changed |= Profile::Update(ProfileKeys::SkyLinesNearTrafficEnabled,
                             settings.near_traffic_enabled,
                             near_traffic_enabled);

  const uint64_t key = ParseUint64(key_text.c_str(), nullptr, 16);
  if (key != settings.key) {
    settings.key = key;
    Profile::Set(ProfileKeys::SkyLinesTrackingKey, key_text.c_str());
    changed = true;
  }

  _changed |= changed;
  return true;
}

#endif /* HAVE_SKYLINES_TRACKING */

std::unique_ptr<Widget>
CreateSkyLinesConfigPanel()
{
#ifdef HAVE_SKYLINES_TRACKING
  return std::make_unique<SkyLinesConfigPanel>();
#else
  return nullptr;
#endif
}
