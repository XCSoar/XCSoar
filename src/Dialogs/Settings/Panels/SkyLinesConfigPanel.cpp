// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkyLinesConfigPanel.hpp"
#include "TrackingIntervalChoices.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "Tracking/Features.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "net/State.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "util/NumberParser.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_SKYLINES_TRACKING

enum ControlIndex {
  SL_ENABLED,
#ifdef HAVE_NET_STATE_ROAMING
  SL_ROAMING,
#endif
  SL_INTERVAL,
  SL_TRAFFIC_ENABLED,
  SL_NEAR_TRAFFIC_ENABLED,
  SL_KEY,
};

class SkyLinesConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  SkyLinesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled);

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

void
SkyLinesConfigPanel::SetEnabled(bool enabled)
{
#ifdef HAVE_NET_STATE_ROAMING
  SetRowEnabled(SL_ROAMING, enabled);
#endif
  SetRowEnabled(SL_INTERVAL, enabled);
  SetRowEnabled(SL_TRAFFIC_ENABLED, enabled);
  SetRowEnabled(SL_NEAR_TRAFFIC_ENABLED,
                enabled && GetValueBoolean(SL_TRAFFIC_ENABLED));
  SetRowEnabled(SL_KEY, enabled);
}

void
SkyLinesConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(SL_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
    return;
  }

  if (IsDataField(SL_TRAFFIC_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetRowEnabled(SL_NEAR_TRAFFIC_ENABLED, dfb.GetValue());
  }
}

void
SkyLinesConfigPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Enable"),
             _("Enable live tracking via the SkyLines server "
               "(tracking.skylines.aero)."),
             settings.skylines.enabled, this);
#ifdef HAVE_NET_STATE_ROAMING
  AddBoolean(_("Roaming"),
             _("Allow tracking when on a roaming mobile data connection."),
             settings.skylines.roaming, this);
#endif
  AddEnum(_("Tracking Interval"), nullptr, tracking_intervals,
          FindClosestTrackingInterval(settings.skylines.interval));

  AddBoolean(_("Track friends"),
             _("Download the position of your SkyLines friends live from "
               "the SkyLines server."),
             settings.skylines.traffic_enabled, this);

  AddBoolean(_("Show nearby traffic"),
             _("Download the position of nearby SkyLines users live from "
               "the SkyLines server."),
             settings.skylines.near_traffic_enabled, this);

  StaticString<64> buffer;
  if (settings.skylines.key != 0)
    buffer.UnsafeFormat("%llX", (unsigned long long)settings.skylines.key);
  else
    buffer.clear();
  AddText(C_("Setting", "Key"),
          _("Your SkyLines tracking key. "
            "This is used to identify your aircraft on the server."),
          buffer);

  SetEnabled(settings.skylines.enabled);
}

static bool
SaveKey(const RowFormWidget &form, unsigned idx, std::string_view profile_key,
        uint64_t &value_r)
{
  const char *const s = form.GetValueString(idx);
  uint64_t value = ParseUint64(s, nullptr, 16);
  if (value == value_r)
    return false;

  value_r = value;
  Profile::Set(profile_key, s);
  return true;
}

bool
SkyLinesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  changed |= SaveValue(SL_ENABLED, ProfileKeys::SkyLinesTrackingEnabled,
                       settings.skylines.enabled);

#ifdef HAVE_NET_STATE_ROAMING
  changed |= SaveValue(SL_ROAMING, ProfileKeys::SkyLinesRoaming,
                       settings.skylines.roaming);
#endif

  changed |= SaveValueEnum(SL_INTERVAL, ProfileKeys::SkyLinesTrackingInterval,
                           settings.skylines.interval);

  changed |= SaveValue(SL_TRAFFIC_ENABLED, ProfileKeys::SkyLinesTrafficEnabled,
                       settings.skylines.traffic_enabled);
  changed |= SaveValue(SL_NEAR_TRAFFIC_ENABLED,
                       ProfileKeys::SkyLinesNearTrafficEnabled,
                       settings.skylines.near_traffic_enabled);

  changed |= SaveKey(*this, SL_KEY, ProfileKeys::SkyLinesTrackingKey,
                     settings.skylines.key);

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
