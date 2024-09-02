// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GaugesConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "MainWindow.hpp"

enum ControlIndex {
  EnableFLARMGauge,
  AutoCloseFlarmDialog,
  AppFlarmLocation,
  TAPosition,
  EnableThermalProfile,
  FinalGlideBarDisplayModeControl,
  EnableFinalGlideBarMC0,
  EnableVarioBar
};

static constexpr StaticEnumChoice final_glide_bar_display_mode_list[] = {
  { FinalGlideBarDisplayMode::OFF, N_("Off"),
    N_("Disable final glide bar.") },
  { FinalGlideBarDisplayMode::ON, N_("On"),
    N_("Always show final glide bar.") },
  { FinalGlideBarDisplayMode::AUTO, N_("Auto"),
    N_("Show final glide bar if approaching final glide range.") },
  nullptr
};

static constexpr StaticEnumChoice flarm_display_location_list[] = {
  { TrafficSettings::GaugeLocation::AUTO,
    N_("Auto (follow infoboxes)") },
  { TrafficSettings::GaugeLocation::TOP_LEFT,
    N_("Top Left") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT,
    N_("Top Right") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT,
    N_("Bottom Left") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT,
    N_("Bottom Right") },
  { TrafficSettings::GaugeLocation::CENTER_TOP,
    N_("Center Top") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM,
    N_("Center Bottom") },
  { TrafficSettings::GaugeLocation::TOP_LEFT_AVOID_IB,
    N_("Top Left (Avoid Infoboxes)") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT_AVOID_IB,
    N_("Top Right (Avoid Infoboxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom Left (Avoid Infoboxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom Right (Avoid Infoboxes)") },
  { TrafficSettings::GaugeLocation::CENTER_TOP_AVOID_IB,
    N_("Center Top (Avoid Infoboxes)") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM_AVOID_IB,
    N_("Center Bottom (Avoid Infoboxes)") },
  nullptr
};

static constexpr StaticEnumChoice thermal_assistant_position_list[] = {
  { UISettings::ThermalAssistantPosition::OFF,
    N_("Off"),
    N_("Disable thermal assistant.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT,
    N_("Bottom left"),
    N_("Show thermal assistant in bottom left.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid infoboxes)"),
    N_("Show thermal assistant in bottom left, above/to right of infoboxes (if there).") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT,
    N_("Bottom right"),
    N_("Show thermal assistant in bottom right.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid infoboxes)"),
    N_("Show thermal assistant in bottom right above/to left of infoboxes (if there).") },
  nullptr
};


class GaugesConfigPanel final : public RowFormWidget, DataFieldListener {
public:
  GaugesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
GaugesConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(FinalGlideBarDisplayModeControl, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    FinalGlideBarDisplayMode fgbdm = (FinalGlideBarDisplayMode)dfe.GetValue();
    SetRowVisible(EnableFinalGlideBarMC0, fgbdm != FinalGlideBarDisplayMode::OFF);
  }
}

void
GaugesConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("FLARM radar"),
             _("This enables the display of the FLARM radar gauge. The track bearing of the target relative to the track bearing of the aircraft is displayed as an arrow head, and a triangle pointing up or down shows the relative altitude of the target relative to you. In all modes, the color of the target indicates the threat level."),
             ui_settings.traffic.enable_gauge);

  AddBoolean(_("Auto close FLARM"),
             _("Setting this to \"On\" will automatically close the FLARM dialog if there is no traffic. \"Off\" will keep the dialog open even without current traffic."),
             ui_settings.traffic.auto_close_dialog);
  SetExpertRow(AutoCloseFlarmDialog);
  
  AddEnum(_("FLARM display"), _("Choose a location for the FLARM display."),
          flarm_display_location_list,
          (unsigned)ui_settings.traffic.gauge_location);
  SetExpertRow(AppFlarmLocation);

  AddEnum(_("Thermal assistant"),
            _("Enable and select the position of the thermal assistant when overlayed on the main screen."),
            thermal_assistant_position_list,
            (unsigned)ui_settings.thermal_assistant_position,
            this);

  AddBoolean(_("Thermal band"),
             _("This enables the display of the thermal profile (climb band) display on the map."),
             map_settings.show_thermal_profile);

  AddEnum(_("Final glide bar"),
          _("If set to \"On\" the final glide will always be shown, if set to \"Auto\" it will be shown when approaching the final glide possibility."),
          final_glide_bar_display_mode_list,
          (unsigned)map_settings.final_glide_bar_display_mode,
          this);
  SetExpertRow(FinalGlideBarDisplayModeControl);

  AddBoolean(_("Final glide bar MC0"),
             _("If set to ON the final glide bar will show a second arrow indicating the required height "
                 "to reach the final waypoint at MC zero."),
             map_settings.final_glide_bar_mc0_enabled);
  SetExpertRow(EnableFinalGlideBarMC0);

  SetRowVisible(EnableFinalGlideBarMC0,
                map_settings.final_glide_bar_display_mode !=
                  FinalGlideBarDisplayMode::OFF);

  AddBoolean(_("Vario bar"),
             _("If set to ON the vario bar will be shown"),
             map_settings.vario_bar_enabled);

  SetExpertRow(EnableVarioBar);
}

bool
GaugesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  changed |= SaveValue(EnableFLARMGauge, ProfileKeys::EnableFLARMGauge,
                       ui_settings.traffic.enable_gauge);

  changed |= SaveValue(AutoCloseFlarmDialog, ProfileKeys::AutoCloseFlarmDialog,
                       ui_settings.traffic.auto_close_dialog);

  if (SaveValueEnum(TAPosition, ProfileKeys::TAPosition,
                    ui_settings.thermal_assistant_position) || 
      SaveValueEnum(AppFlarmLocation, ProfileKeys::FlarmLocation,
                    ui_settings.traffic.gauge_location))
    CommonInterface::main_window->ReinitialiseLayout();

  changed |= SaveValue(EnableThermalProfile, ProfileKeys::EnableThermalProfile,
                       map_settings.show_thermal_profile);

  changed |= SaveValueEnum(FinalGlideBarDisplayModeControl,
                           ProfileKeys::FinalGlideBarDisplayMode,
                           map_settings.final_glide_bar_display_mode);

  changed |= SaveValue(EnableFinalGlideBarMC0, ProfileKeys::EnableFinalGlideBarMC0,
                       map_settings.final_glide_bar_mc0_enabled);

  changed |= SaveValue(EnableVarioBar, ProfileKeys::EnableVarioBar,
                       map_settings.vario_bar_enabled);
  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateGaugesConfigPanel()
{
  return std::make_unique<GaugesConfigPanel>();
}
