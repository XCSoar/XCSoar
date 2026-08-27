// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "MainWindow.hpp"

enum ControlIndex {
  TrafficRadar,
  AutoCloseFlarmDialog,
  NoPositionTargetDistanceRing,
};

/** Not a GaugeLocation; used only in this panel's Off/position enum. */
static constexpr unsigned TRAFFIC_RADAR_OFF = ~0u;

static constexpr StaticEnumChoice traffic_radar_list[] = {
  { TRAFFIC_RADAR_OFF, N_("Off"),
    N_("Disable the traffic radar.") },
  { TrafficSettings::GaugeLocation::AUTO,
    N_("Auto (follow InfoBoxes)") },
  { TrafficSettings::GaugeLocation::TOP_LEFT,
    N_("Top left") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT,
    N_("Top right") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT,
    N_("Bottom left") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT,
    N_("Bottom right") },
  { TrafficSettings::GaugeLocation::CENTER_TOP,
    N_("Center top") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM,
    N_("Center bottom") },
  { TrafficSettings::GaugeLocation::TOP_LEFT_AVOID_IB,
    N_("Top left (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT_AVOID_IB,
    N_("Top right (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::CENTER_TOP_AVOID_IB,
    N_("Center top (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM_AVOID_IB,
    N_("Center bottom (avoid InfoBoxes)") },
  nullptr
};

class TrafficConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  TrafficConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void SetRadarEnabled(bool enabled) noexcept;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
TrafficConfigPanel::SetRadarEnabled(bool enabled) noexcept
{
  SetRowEnabled(NoPositionTargetDistanceRing, enabled);
}

void
TrafficConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(TrafficRadar, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    SetRadarEnabled(dfe.GetValue() != TRAFFIC_RADAR_OFF);
  }
}

void
TrafficConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  const unsigned radar = ui_settings.traffic.enable_gauge
    ? unsigned(ui_settings.traffic.gauge_location)
    : TRAFFIC_RADAR_OFF;

  AddEnum(_("FLARM Radar"),
          _("Enable the traffic radar and choose its position on the "
            "main screen. The track bearing of the target relative to the "
            "track bearing of the aircraft is displayed as an arrow head, "
            "and a triangle pointing up or down shows the relative altitude "
            "of the target relative to you. In all modes, the color of the "
            "target indicates the threat level."),
          traffic_radar_list, radar, this);

  AddBoolean(_("Auto close FLARM"),
             _("Setting this to \"On\" will automatically close the FLARM dialog if there is no traffic. \"Off\" will keep the dialog open even without current traffic."),
             ui_settings.traffic.auto_close_dialog);
  SetExpertRow(AutoCloseFlarmDialog);

  AddBoolean(_("No position target"),
             _("This parameter enables or disables the No Position Target Distance Ring in Flarm Radar"),
             ui_settings.traffic.no_position_target_distance_ring);

  SetRadarEnabled(ui_settings.traffic.enable_gauge);
}

bool
TrafficConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  const unsigned radar = GetValueEnum(TrafficRadar);
  const bool enable = radar != TRAFFIC_RADAR_OFF;

  if (enable != ui_settings.traffic.enable_gauge) {
    ui_settings.traffic.enable_gauge = enable;
    Profile::Set(ProfileKeys::EnableFLARMGauge, enable);
    changed = true;
  }

  if (enable) {
    const auto location =
      static_cast<TrafficSettings::GaugeLocation>(radar);
    if (location != ui_settings.traffic.gauge_location) {
      ui_settings.traffic.gauge_location = location;
      Profile::SetEnum(ProfileKeys::FlarmLocation, location);
      CommonInterface::main_window->ReinitialiseLayout();
      changed = true;
    }
  }

  changed |= SaveValue(AutoCloseFlarmDialog, ProfileKeys::AutoCloseFlarmDialog,
                       ui_settings.traffic.auto_close_dialog);

  changed |= SaveValue(NoPositionTargetDistanceRing,
                       ProfileKeys::NoPositionTargetDistanceRing,
                       ui_settings.traffic.no_position_target_distance_ring);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTrafficConfigPanel()
{
  return std::make_unique<TrafficConfigPanel>();
}
