// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LiveTrack24ConfigPanel.hpp"
#include "TrackingIntervalChoices.hpp"
#include "Profile/Keys.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "Tracking/Features.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#ifdef HAVE_LIVETRACK24

enum ControlIndex {
  LT24_ENABLED,
  LT24_INVERVAL,
  LT24_VEHICLE_TYPE,
  LT24_VEHICLE_NAME,
  LT24_SERVER,
  LT24_USERNAME,
  LT24_PASSWORD,
};

static constexpr StaticEnumChoice server_list[] = {
  { 0, "www.livetrack24.com" },
  { 1, "test.livetrack24.com" },
  { 2, "livexc.dhv.de" },
  nullptr,
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

class LiveTrack24ConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  LiveTrack24ConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled);

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

void
LiveTrack24ConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(LT24_INVERVAL, enabled);
  SetRowEnabled(LT24_VEHICLE_TYPE, enabled);
  SetRowEnabled(LT24_VEHICLE_NAME, enabled);
  SetRowEnabled(LT24_SERVER, enabled);
  SetRowEnabled(LT24_USERNAME, enabled);
  SetRowEnabled(LT24_PASSWORD, enabled);
}

void
LiveTrack24ConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(LT24_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
}

void
LiveTrack24ConfigPanel::Prepare(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Enable"), "", settings.livetrack24.enabled, this);

  AddEnum(_("Tracking Interval"), nullptr, tracking_intervals,
          FindClosestTrackingInterval(settings.livetrack24.interval));

  AddEnum(_("Vehicle Type"), _("Type of vehicle used."), vehicle_type_list,
          (unsigned)settings.livetrack24.vehicleType);
  AddText(_("Vehicle Name"), _("Name of vehicle used."),
          settings.livetrack24.vehicle_name);

  WndProperty *edit = AddEnum(_("Server"), "", server_list, 0);
  ((DataFieldEnum *)edit->GetDataField())->SetValue(
    settings.livetrack24.server);
  edit->RefreshDisplay();

  AddText(_("Username"), "", settings.livetrack24.username);
  AddPassword(_("Password"), "", settings.livetrack24.password);

  SetEnabled(settings.livetrack24.enabled);
}

bool
LiveTrack24ConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  changed |= SaveValue(LT24_ENABLED, ProfileKeys::LiveTrack24Enabled,
                       settings.livetrack24.enabled);

  changed |= SaveValueEnum(LT24_INVERVAL,
                           ProfileKeys::LiveTrack24TrackingInterval,
                           settings.livetrack24.interval);

  changed |= SaveValueEnum(LT24_VEHICLE_TYPE,
                           ProfileKeys::LiveTrack24TrackingVehicleType,
                           settings.livetrack24.vehicleType);

  changed |= SaveValue(LT24_VEHICLE_NAME,
                       ProfileKeys::LiveTrack24TrackingVehicleName,
                       settings.livetrack24.vehicle_name);

  changed |= SaveValue(LT24_SERVER, ProfileKeys::LiveTrack24Server,
                       settings.livetrack24.server);

  changed |= SaveValue(LT24_USERNAME, ProfileKeys::LiveTrack24Username,
                       settings.livetrack24.username);

  changed |= SaveValue(LT24_PASSWORD, ProfileKeys::LiveTrack24Password,
                       settings.livetrack24.password);

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
