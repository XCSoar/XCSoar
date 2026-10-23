// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PureTrackConfigPanel.hpp"
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

#ifdef HAVE_PURETRACK

enum ControlIndex {
  PT_ENABLED,
  PT_INTERVAL,
  PT_VEHICLE_TYPE,
  PT_LABEL,
  PT_ENDPOINT,
  PT_APP_KEY,
  PT_DEVICE_ID,
};

static constexpr StaticEnumChoice vehicle_type_list[] = {
  { PureTrack::Settings::VehicleType::GLIDER, N_("Glider") },
  { PureTrack::Settings::VehicleType::PARAGLIDER, N_("Paraglider") },
  { PureTrack::Settings::VehicleType::POWERED_AIRCRAFT,
    N_("Powered aircraft") },
  { PureTrack::Settings::VehicleType::HOT_AIR_BALLOON,
    N_("Hot-air balloon") },
  { PureTrack::Settings::VehicleType::HANGGLIDER_FLEX,
    N_("Hangglider (Flex/FAI1)") },
  { PureTrack::Settings::VehicleType::HANGGLIDER_RIGID,
    N_("Hangglider (Rigid/FAI5)") },
  nullptr,
};

class PureTrackConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  PureTrackConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled);

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

void
PureTrackConfigPanel::SetEnabled(bool enabled)
{
  SetRowEnabled(PT_INTERVAL, enabled);
  SetRowEnabled(PT_VEHICLE_TYPE, enabled);
  SetRowEnabled(PT_LABEL, enabled);
  SetRowEnabled(PT_ENDPOINT, enabled);
  SetRowEnabled(PT_APP_KEY, enabled);
  SetRowEnabled(PT_DEVICE_ID, enabled);
}

void
PureTrackConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(PT_ENABLED, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
}

void
PureTrackConfigPanel::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Enable"), "", settings.puretrack.enabled, this);

  AddTrackingIntervalRow(*this, settings.puretrack.interval);

  AddEnum(_("Vehicle Type"), _("Type of vehicle used."), vehicle_type_list,
          (unsigned)settings.puretrack.vehicle_type);
  AddText(_("Vehicle Name"), _("Name shown by PureTrack."),
          settings.puretrack.label);
  AddText(_("Server"), _("PureTrack Insert API endpoint URL."),
          settings.puretrack.endpoint);
  AddPassword(C_("Setting", "Key"),
              _("PureTrack Insert API application key."),
              settings.puretrack.app_key);
  AddText(_("Device ID"), _("Unique device identifier sent to PureTrack."),
          settings.puretrack.device_id);

  SetEnabled(settings.puretrack.enabled);
}

bool
PureTrackConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  changed |= SaveValue(PT_ENABLED, ProfileKeys::PureTrackEnabled,
                       settings.puretrack.enabled);

  changed |= SaveValueEnum(PT_INTERVAL, ProfileKeys::PureTrackInsertInterval,
                           settings.puretrack.interval);

  changed |= SaveValueEnum(PT_VEHICLE_TYPE,
                           ProfileKeys::PureTrackInsertVehicleType,
                           settings.puretrack.vehicle_type);

  changed |= SaveValue(PT_LABEL, ProfileKeys::PureTrackInsertLabel,
                       settings.puretrack.label);

  changed |= SaveValue(PT_ENDPOINT, ProfileKeys::PureTrackInsertEndpoint,
                       settings.puretrack.endpoint);

  changed |= SaveValue(PT_APP_KEY, ProfileKeys::PureTrackInsertAppKey,
                       settings.puretrack.app_key);

  changed |= SaveValue(PT_DEVICE_ID, ProfileKeys::PureTrackInsertDeviceID,
                       settings.puretrack.device_id);

  _changed |= changed;
  return true;
}

#endif /* HAVE_PURETRACK */

std::unique_ptr<Widget>
CreatePureTrackConfigPanel()
{
#ifdef HAVE_PURETRACK
  return std::make_unique<PureTrackConfigPanel>();
#else
  return nullptr;
#endif
}
