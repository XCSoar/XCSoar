// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Dialogs/NumberEntry.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Tracking/CloudSettings.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "net/State.hpp"
#include "util/StringStrip.hxx"

#include <fmt/format.h>

#include <stdio.h>
#include <string>
#include <string_view>

/**
 * The XCSoar Cloud: whether to take part, what to receive, and the
 * server.
 */
class CloudConfigPanel final : public ConfigListPanel {
  bool enabled, show_traffic;
#ifdef HAVE_NET_STATE_ROAMING
  bool roaming;
#endif
  bool show_thermals;
  StaticString<64> host;
  unsigned port;
  StaticString<CloudSettings::OWN_FLARM_IDS_TEXT_SIZE> own_flarm_ids;

  /** Help text for Own FLARM IDs (holds fmt result). */
  std::string own_flarm_help;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
CloudConfigPanel::LoadSettings() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.cloud;

  enabled = settings.enabled == TriState::TRUE;
  show_traffic = settings.show_traffic;
#ifdef HAVE_NET_STATE_ROAMING
  roaming = settings.roaming;
#endif
  show_thermals = settings.show_thermals;
  host = settings.host;
  port = settings.port;

  CloudSettings::FormatOwnFlarmIds(settings.own_flarm_ids,
                                   own_flarm_ids.buffer(),
                                   own_flarm_ids.capacity());

  own_flarm_help = fmt::format(
    fmt::runtime(
      _("Comma-separated hex FLARM / ICAO addresses (up to {}) "
        "used to hide your own aircraft from OGN traffic. Use "
        "this when the FLARM radio id is not available (for "
        "example behind an LX passthrough), or to hide "
        "additional own aircraft. Leave empty to use the "
        "device id when known.")),
    CloudSettings::MAX_OWN_FLARM_IDS);
}

void
CloudConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem("XCSoar Cloud",
                _("Participate in the XCSoar Cloud? This transmits your "
                  "position while flying and allows receiving traffic from "
                  "other XCSoar Cloud participants and OGN, as well as "
                  "thermal and wave locations from the cloud server."),
                enabled);

  /* what is received; greyed out while the cloud is off */
  AddGroup();

  AddToggleItem(C_("Setting", "Show traffic"),
                _("Receive traffic from the XCSoar Cloud server and OGN. "
                  "Requires flying with a real GPS fix."),
                show_traffic, nullptr, !enabled);

#ifdef HAVE_NET_STATE_ROAMING
  AddToggleItem(_("Roaming"),
                _("Allow XCSoar Cloud communication when on a roaming "
                  "mobile data connection."),
                roaming, nullptr, !enabled);
#endif

  AddToggleItem(_("Show thermals"),
                _("Obtain and show thermal locations reported by others."),
                show_thermals, nullptr, !enabled);

  if (!IsExpert())
    return;

  /* the server */
  AddGroup();

  StaticString<16> port_text;
  port_text.Format("%u", port);

  if (enabled) {
    AddTextItem(_("Server"),
                _("Hostname or IP address of the XCSoar Cloud server."),
                host);

    AddItem(_("Port"), [this](){
      unsigned value = port;
      if (NumberEntryDialog(_("Port"), value, 5) && value != port &&
          value >= 1 && value <= 65535) {
        port = value;
        Refresh();
      }
    }, {.value = port_text.c_str(), .chevron = true,
        .help = _("UDP port of the XCSoar Cloud server.")});

    AddTextItem(C_("Setting", "Own FLARM IDs"), own_flarm_help.c_str(),
                own_flarm_ids);
  } else {
    AddItem(_("Server"), {.value = host.c_str(), .disabled = true});
    AddItem(_("Port"), {.value = port_text.c_str(), .disabled = true});
    AddItem(C_("Setting", "Own FLARM IDs"),
            {.value = own_flarm_ids.c_str(), .disabled = true});
  }
}

bool
CloudConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.cloud;

  if (enabled != (settings.enabled == TriState::TRUE)) {
    settings.enabled = enabled ? TriState::TRUE : TriState::FALSE;
    Profile::Set(ProfileKeys::CloudEnabled, enabled);

    if (settings.enabled == TriState::TRUE && settings.key == 0) {
      settings.key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::CloudShowTraffic,
                             settings.show_traffic, show_traffic);

#ifdef HAVE_NET_STATE_ROAMING
  changed |= Profile::Update(ProfileKeys::CloudRoaming,
                             settings.roaming, roaming);
#endif

  changed |= Profile::Update(ProfileKeys::CloudShowThermals,
                             settings.show_thermals, show_thermals);

  if (Profile::Update(ProfileKeys::CloudHost, settings.host, host)) {
    if (settings.host.empty())
      settings.host = CloudSettings::DEFAULT_HOST;
    changed = true;
  }

  if (Profile::Update(ProfileKeys::CloudPort, settings.port, port)) {
    if (settings.port == 0 || settings.port > 65535u)
      settings.port = CloudSettings::DEFAULT_PORT;
    changed = true;
  }

  {
    const auto ids =
      CloudSettings::ParseOwnFlarmIds(own_flarm_ids.c_str());
    const bool input_blank =
      Strip(std::string_view{own_flarm_ids.c_str()}).empty();

    /* Non-empty garbage must not wipe a previously valid list. */
    if (input_blank || !ids.empty()) {
      bool same = settings.own_flarm_ids.size() == ids.size();
      for (unsigned i = 0; same && i < ids.size(); ++i)
        same = settings.own_flarm_ids[i] == ids[i];

      if (!same) {
        settings.own_flarm_ids = ids;
        char tmp[CloudSettings::OWN_FLARM_IDS_TEXT_SIZE];
        CloudSettings::FormatOwnFlarmIds(ids, tmp, sizeof(tmp));
        Profile::Set(ProfileKeys::CloudOwnFlarmId, tmp);
        changed = true;
      }
    }
  }

  _changed |= changed;

#ifdef HAVE_TRACKING
  if (changed && net_components != nullptr &&
      net_components->tracking != nullptr)
    net_components->tracking->SetSettings(
      CommonInterface::GetComputerSettings().tracking);
#endif

  return true;
}

std::unique_ptr<Widget>
CreateCloudConfigPanel()
{
  return std::make_unique<CloudConfigPanel>();
}
