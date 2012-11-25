/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/DataField/Base.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Util/NumberParser.hpp"

enum ControlIndex {
  SL_ENABLED,
  SL_INTERVAL,
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  SL_TRAFFIC_ENABLED,
#endif
  SL_KEY,
  SPACER,
  LT24Enabled,
  TrackingInterval,
  TrackingVehicleType,
  LT24Server,
  LT24Username,
  LT24Password
};

class TrackingConfigPanel
  : public RowFormWidget, DataFieldListener {
public:
  TrackingConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void SetSkyLinesEnabled(bool enabled);
  void SetEnabled(bool enabled);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);
};

void
TrackingConfigPanel::SetSkyLinesEnabled(bool enabled)
{
  SetRowEnabled(SL_INTERVAL, enabled);
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  SetRowEnabled(SL_TRAFFIC_ENABLED, enabled);
#endif
  SetRowEnabled(SL_KEY, enabled);
}

void
TrackingConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(TrackingInterval, enabled);
  SetRowEnabled(TrackingVehicleType, enabled);
  SetRowEnabled(LT24Server, enabled);
  SetRowEnabled(LT24Username, enabled);
  SetRowEnabled(LT24Password, enabled);
}

void
TrackingConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(SL_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetSkyLinesEnabled(dfb.GetAsBoolean());
  } else if (IsDataField(LT24Enabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetAsBoolean());
  }
}

static constexpr StaticEnumChoice server_list[] = {
  { 0, _T("www.livetrack24.com") },
  { 1, _T("test.livetrack24.com") },
  { 2, _T("livexc.dhv1.de") },
  { 0 },
};

static constexpr StaticEnumChoice vehicle_type_list[] = {
  { (unsigned) TrackingSettings::VehicleType::GLIDER, N_("Glider") },
  { (unsigned) TrackingSettings::VehicleType::PARAGLIDER, N_("Paraglider") },
  { (unsigned) TrackingSettings::VehicleType::POWERED_AIRCRAFT, N_("Powered aircraft") },
  { (unsigned) TrackingSettings::VehicleType::HOT_AIR_BALLOON, N_("Hot-air balloon") },
  { 0 },
};

void
TrackingConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_T("SkyLines"), NULL, settings.skylines.enabled, this);
  AddTime(_T("Tracking Interval"), NULL, 5, 1200, 5,
          settings.skylines.interval);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  AddBoolean(_("Track friends"),
             _("Download the position of your friends live from the SkyLines server."),
             settings.skylines.traffic_enabled, this);
#endif

  StaticString<64> buffer;
  if (settings.skylines.key != 0)
    buffer.UnsafeFormat(_T("%llX"), (unsigned long long)settings.skylines.key);
  else
    buffer.clear();
  AddText(_T("Key"), NULL, buffer);

  AddSpacer();

  AddBoolean(_T("LiveTrack24"),  _T(""), settings.livetrack24.enabled, this);

  AddTime(_("Tracking Interval"), _T(""), 5, 3600, 5, settings.interval);

  AddEnum(_("Vehicle Type"), _("Type of vehicle used."), vehicle_type_list,
          (unsigned) settings.vehicleType);

  WndProperty *edit = AddEnum(_("Server"), _T(""), server_list, 0);
  ((DataFieldEnum *)edit->GetDataField())->Set(settings.livetrack24.server);
  edit->RefreshDisplay();

  AddText(_("Username"), _T(""), settings.livetrack24.username);
  AddPassword(_("Password"), _T(""), settings.livetrack24.password);

  SetSkyLinesEnabled(settings.skylines.enabled);
  SetEnabled(settings.livetrack24.enabled);
}

static bool
SaveKey(const RowFormWidget &form, unsigned idx, const TCHAR *profile_key,
        uint64_t &value_r)
{
  const TCHAR *const s = form.GetValueString(idx);
  uint64_t value = ParseUint64(s, NULL, 16);
  if (value == value_r)
    return false;

  value_r = value;
  Profile::Set(profile_key, s);
  return true;
}

bool
TrackingConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  changed |= SaveValue(TrackingInterval, ProfileKeys::TrackingInterval, settings.interval);

  changed |= SaveValueEnum(TrackingVehicleType, ProfileKeys::TrackingVehicleType,
                           settings.vehicleType);

  changed |= SaveValue(SL_ENABLED, ProfileKeys::SkyLinesTrackingEnabled,
                       settings.skylines.enabled);

  changed |= SaveValue(SL_INTERVAL, ProfileKeys::SkyLinesTrackingInterval,
                       settings.skylines.interval);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  changed |= SaveValue(SL_TRAFFIC_ENABLED, ProfileKeys::SkyLinesTrafficEnabled,
                       settings.skylines.traffic_enabled);
#endif

  changed |= SaveKey(*this, SL_KEY, ProfileKeys::SkyLinesTrackingKey,
                     settings.skylines.key);

  changed |= SaveValue(LT24Enabled, ProfileKeys::LiveTrack24Enabled, settings.livetrack24.enabled);

  changed |= SaveValue(LT24Server, ProfileKeys::LiveTrack24Server,
                       settings.livetrack24.server.buffer(), settings.livetrack24.server.MAX_SIZE);

  changed |= SaveValue(LT24Username, ProfileKeys::LiveTrack24Username,
                       settings.livetrack24.username.buffer(), settings.livetrack24.username.MAX_SIZE);

  changed |= SaveValue(LT24Password, ProfileKeys::LiveTrack24Password,
                       settings.livetrack24.password.buffer(), settings.livetrack24.password.MAX_SIZE);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateTrackingConfigPanel()
{
  return new TrackingConfigPanel();
}
