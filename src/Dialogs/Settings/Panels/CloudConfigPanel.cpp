// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "net/State.hpp"

#include <stdio.h>

enum ControlIndex {
  ENABLED,
  SHOW_TRAFFIC,
#ifdef HAVE_NET_STATE_ROAMING
  ROAMING,
#endif
  SHOW_THERMALS,
};

class CloudConfigPanel final
  : public RowFormWidget, DataFieldListener {
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

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateCloudConfigPanel()
{
  return std::make_unique<CloudConfigPanel>();
}
