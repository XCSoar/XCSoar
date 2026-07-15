// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Tracking/CloudSettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "net/State.hpp"
#include "util/StringStrip.hxx"

#include <fmt/format.h>

#include <stdio.h>
#include <string>
#include <string_view>

enum ControlIndex {
  ENABLED,
  SHOW_TRAFFIC,
#ifdef HAVE_NET_STATE_ROAMING
  ROAMING,
#endif
  SHOW_THERMALS,
  HOST,
  PORT,
  OWN_FLARM_ID,
};

class CloudConfigPanel final
  : public RowFormWidget, DataFieldListener {
  /** Help text for Own FLARM IDs (holds fmt result for SetHelpText). */
  std::string own_flarm_help;

public:
  CloudConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
CloudConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(SHOW_TRAFFIC, enabled);
#ifdef HAVE_NET_STATE_ROAMING
  SetRowEnabled(ROAMING, enabled);
#endif
  SetRowEnabled(SHOW_THERMALS, enabled);
  SetRowEnabled(HOST, enabled);
  SetRowEnabled(PORT, enabled);
  SetRowEnabled(OWN_FLARM_ID, enabled);
}

void
CloudConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
}

void
CloudConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.cloud;

  AddBoolean("XCSoar Cloud",
             _("Participate in the XCSoar Cloud? This transmits your "
               "position while flying and allows receiving traffic from "
               "other XCSoar Cloud participants and OGN, as well as "
               "thermal and wave locations from the cloud server."),
             settings.enabled == TriState::TRUE,
             this);

  AddBoolean(_("Show traffic"),
             _("Receive traffic from the XCSoar Cloud server and OGN. "
               "Requires flying with a real GPS fix."),
             settings.show_traffic, this);

#ifdef HAVE_NET_STATE_ROAMING
  AddBoolean(_("Roaming"),
             _("Allow XCSoar Cloud communication when on a roaming "
               "mobile data connection."),
             settings.roaming, this);
#endif

  AddBoolean(_("Show thermals"),
             _("Obtain and show thermal locations reported by others."),
             settings.show_thermals);

  AddText(_("Server"),
          _("Hostname or IP address of the XCSoar Cloud server."),
          settings.host.c_str());
  SetExpertRow(HOST);

  AddInteger(_("Port"),
             _("UDP port of the XCSoar Cloud server."),
             "%u", "%u",
             1, 65535, 1, int(settings.port));
  SetExpertRow(PORT);

  char own_flarm_text[CloudSettings::OWN_FLARM_IDS_TEXT_SIZE] = "";
  CloudSettings::FormatOwnFlarmIds(settings.own_flarm_ids,
                                   own_flarm_text, sizeof(own_flarm_text));
  own_flarm_help = fmt::format(
    fmt::runtime(
      _("Comma-separated hex FLARM / ICAO addresses (up to {}) "
        "used to hide your own aircraft from OGN traffic. Use "
        "this when the FLARM radio id is not available (for "
        "example behind an LX passthrough), or to hide "
        "additional own aircraft. Leave empty to use the "
        "device id when known.")),
    CloudSettings::MAX_OWN_FLARM_IDS);
  AddText(_("Own FLARM IDs"),
          own_flarm_help.c_str(),
          own_flarm_text);
  SetExpertRow(OWN_FLARM_ID);

  SetEnabled(settings.enabled == TriState::TRUE);
}

bool
CloudConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.cloud;

  bool cloud_enabled = settings.enabled == TriState::TRUE;
  if (SaveValue(ENABLED, ProfileKeys::CloudEnabled, cloud_enabled)) {
    settings.enabled = cloud_enabled
      ? TriState::TRUE
      : TriState::FALSE;

    if (settings.enabled == TriState::TRUE && settings.key == 0) {
      settings.key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    changed = true;
  }

  changed |= SaveValue(SHOW_TRAFFIC, ProfileKeys::CloudShowTraffic,
                       settings.show_traffic);

#ifdef HAVE_NET_STATE_ROAMING
  changed |= SaveValue(ROAMING, ProfileKeys::CloudRoaming,
                       settings.roaming);
#endif

  changed |= SaveValue(SHOW_THERMALS, ProfileKeys::CloudShowThermals,
                       settings.show_thermals);

  if (SaveValue(HOST, ProfileKeys::CloudHost, settings.host)) {
    if (settings.host.empty())
      settings.host = CloudSettings::DEFAULT_HOST;
    changed = true;
  }

  if (SaveValueInteger(PORT, ProfileKeys::CloudPort, settings.port)) {
    if (settings.port == 0 || settings.port > 65535u)
      settings.port = CloudSettings::DEFAULT_PORT;
    changed = true;
  }

  StaticString<CloudSettings::OWN_FLARM_IDS_TEXT_SIZE> own_flarm_text;
  if (SaveValue(OWN_FLARM_ID, own_flarm_text)) {
    const auto ids =
      CloudSettings::ParseOwnFlarmIds(own_flarm_text.c_str());
    const bool input_blank = Strip(std::string_view{own_flarm_text.c_str()})
      .empty();

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
  if (changed && net_components != nullptr && net_components->tracking != nullptr)
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
