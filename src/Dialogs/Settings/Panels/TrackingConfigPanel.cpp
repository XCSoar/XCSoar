/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TrackingConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Net/State.hpp"
#include "Form/DataField/Base.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Util/NumberParser.hpp"

enum ControlIndex {
#ifdef HAVE_SKYLINES_TRACKING
  SL_ENABLED,
#ifdef HAVE_NET_STATE_ROAMING
  SL_ROAMING,
#endif
  SL_INTERVAL,
  SL_TRAFFIC_ENABLED,
  SL_NEAR_TRAFFIC_ENABLED,
  SL_KEY,
#endif
#if defined(HAVE_SKYLINES_TRACKING) && defined(HAVE_LIVETRACK24)
  SPACER,
#endif
#ifdef HAVE_LIVETRACK24
  LT24Enabled,
  TrackingInterval,
  TrackingVehicleType,
  TrackingVehicleName,
  LT24Server,
  LT24Username,
  LT24Password
#endif
};

class TrackingConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  TrackingConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
#ifdef HAVE_SKYLINES_TRACKING
  void SetSkyLinesEnabled(bool enabled);
#endif

#ifdef HAVE_LIVETRACK24
  void SetEnabled(bool enabled);
#endif

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

#ifdef HAVE_SKYLINES_TRACKING

void
TrackingConfigPanel::SetSkyLinesEnabled(bool enabled)
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

#endif

#ifdef HAVE_LIVETRACK24

void
TrackingConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(TrackingInterval, enabled);
  SetRowEnabled(TrackingVehicleType, enabled);
  SetRowEnabled(TrackingVehicleName, enabled);
  SetRowEnabled(LT24Server, enabled);
  SetRowEnabled(LT24Username, enabled);
  SetRowEnabled(LT24Password, enabled);
}

#endif

void
TrackingConfigPanel::OnModified(DataField &df)
{
#ifdef HAVE_SKYLINES_TRACKING
  if (IsDataField(SL_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetSkyLinesEnabled(dfb.GetAsBoolean());
    return;
  }

  if (IsDataField(SL_TRAFFIC_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetRowEnabled(SL_NEAR_TRAFFIC_ENABLED, dfb.GetAsBoolean());
    return;
  }
#endif

#ifdef HAVE_LIVETRACK24
  if (IsDataField(LT24Enabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetAsBoolean());
  }
#endif
}

#ifdef HAVE_SKYLINES_TRACKING

static constexpr StaticEnumChoice tracking_intervals[] = {
  { 1, _T("1 sec") },
  { 2, _T("2 sec") },
  { 3, _T("3 sec") },
  { 5, _T("5 sec") },
  { 10, _T("10 sec") },
  { 15, _T("15 sec") },
  { 20, _T("20 sec") },
  { 30, _T("30 sec") },
  { 45, _T("45 sec") },
  { 60, _T("1 min") },
  { 120, _T("2 min") },
  { 180, _T("3 min") },
  { 300, _T("5 min") },
  { 600, _T("10 min") },
  { 900, _T("15 min") },
  { 1200, _T("20 min") },
  { 0 },
};

#endif

#ifdef HAVE_LIVETRACK24

static constexpr StaticEnumChoice server_list[] = {
  { 0, _T("www.livetrack24.com") },
  { 1, _T("test.livetrack24.com") },
  { 2, _T("livexc.dhv.de") },
  { 0 },
};

static constexpr StaticEnumChoice vehicle_type_list[] = {
  { (unsigned) TrackingSettings::VehicleType::GLIDER, N_("Glider") },
  { (unsigned) TrackingSettings::VehicleType::PARAGLIDER, N_("Paraglider") },
  { (unsigned) TrackingSettings::VehicleType::POWERED_AIRCRAFT, N_("Powered aircraft") },
  { (unsigned) TrackingSettings::VehicleType::HOT_AIR_BALLOON, N_("Hot-air balloon") },
  { (unsigned) TrackingSettings::VehicleType::HANGGLIDER_FLEX, N_("Hangglider (Flex/FAI1)") },
  { (unsigned) TrackingSettings::VehicleType::HANGGLIDER_RIGID, N_("Hangglider (Rigid/FAI5)") },
  { 0 },
};

#endif

void
TrackingConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_SKYLINES_TRACKING
  AddBoolean(_T("SkyLines"), nullptr, settings.skylines.enabled, this);
#ifdef HAVE_NET_STATE_ROAMING
  AddBoolean(_T("Roaming"), nullptr, settings.skylines.roaming, this);
#endif
  AddEnum(_("Tracking Interval"), nullptr, tracking_intervals,
          settings.skylines.interval);

  AddBoolean(_("Track friends"),
             _("Download the position of your friends live from the SkyLines server."),
             settings.skylines.traffic_enabled, this);

  AddBoolean(_("Show nearby traffic"),
             _("Download the position of your nearby traffic live from the SkyLines server."),
             settings.skylines.near_traffic_enabled, this);

  StaticString<64> buffer;
  if (settings.skylines.key != 0)
    buffer.UnsafeFormat(_T("%llX"), (unsigned long long)settings.skylines.key);
  else
    buffer.clear();
  AddText(_T("Key"), nullptr, buffer);
#endif

#if defined(HAVE_SKYLINES_TRACKING) && defined(HAVE_LIVETRACK24)
  AddSpacer();
#endif

#ifdef HAVE_LIVETRACK24
  AddBoolean(_T("LiveTrack24"),  _T(""), settings.livetrack24.enabled, this);

  AddTime(_("Tracking Interval"), _T(""), 5, 3600, 5, settings.interval);

  AddEnum(_("Vehicle Type"), _("Type of vehicle used."), vehicle_type_list,
          (unsigned) settings.vehicleType);
  AddText(_("Vehicle Name"), _T("Name of vehicle used."),
          settings.vehicle_name);

  WndProperty *edit = AddEnum(_("Server"), _T(""), server_list, 0);
  ((DataFieldEnum *)edit->GetDataField())->Set(settings.livetrack24.server);
  edit->RefreshDisplay();

  AddText(_("Username"), _T(""), settings.livetrack24.username);
  AddPassword(_("Password"), _T(""), settings.livetrack24.password);
#endif

#ifdef HAVE_SKYLINES_TRACKING
  SetSkyLinesEnabled(settings.skylines.enabled);
#endif

#ifdef HAVE_LIVETRACK24
  SetEnabled(settings.livetrack24.enabled);
#endif
}

#ifdef HAVE_SKYLINES_TRACKING
static bool
SaveKey(const RowFormWidget &form, unsigned idx, const char *profile_key,
        uint64_t &value_r)
{
  const TCHAR *const s = form.GetValueString(idx);
  uint64_t value = ParseUint64(s, nullptr, 16);
  if (value == value_r)
    return false;

  value_r = value;
  Profile::Set(profile_key, s);
  return true;
}
#endif

bool
TrackingConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

#ifdef HAVE_LIVETRACK24
  changed |= SaveValue(TrackingInterval, ProfileKeys::TrackingInterval, settings.interval);

  changed |= SaveValueEnum(TrackingVehicleType, ProfileKeys::TrackingVehicleType,
                           settings.vehicleType);

  changed |= SaveValue(TrackingVehicleName, ProfileKeys::TrackingVehicleName,
                       settings.vehicle_name);
#endif

#ifdef HAVE_SKYLINES_TRACKING
  changed |= SaveValue(SL_ENABLED, ProfileKeys::SkyLinesTrackingEnabled,
                       settings.skylines.enabled);

#ifdef HAVE_NET_STATE_ROAMING
  changed |= SaveValue(SL_ROAMING, ProfileKeys::SkyLinesRoaming,
                       settings.skylines.roaming);
#endif

  changed |= SaveValue(SL_INTERVAL, ProfileKeys::SkyLinesTrackingInterval,
                       settings.skylines.interval);

  changed |= SaveValue(SL_TRAFFIC_ENABLED, ProfileKeys::SkyLinesTrafficEnabled,
                       settings.skylines.traffic_enabled);
  changed |= SaveValue(SL_NEAR_TRAFFIC_ENABLED,
                       ProfileKeys::SkyLinesNearTrafficEnabled,
                       settings.skylines.near_traffic_enabled);

  changed |= SaveKey(*this, SL_KEY, ProfileKeys::SkyLinesTrackingKey,
                     settings.skylines.key);
#endif

#ifdef HAVE_LIVETRACK24
  changed |= SaveValue(LT24Enabled, ProfileKeys::LiveTrack24Enabled, settings.livetrack24.enabled);

  changed |= SaveValue(LT24Server, ProfileKeys::LiveTrack24Server,
                       settings.livetrack24.server);

  changed |= SaveValue(LT24Username, ProfileKeys::LiveTrack24Username,
                       settings.livetrack24.username);

  changed |= SaveValue(LT24Password, ProfileKeys::LiveTrack24Password,
                       settings.livetrack24.password);
#endif

  _changed |= changed;

  return true;
}

Widget *
CreateTrackingConfigPanel()
{
  return new TrackingConfigPanel();
}
