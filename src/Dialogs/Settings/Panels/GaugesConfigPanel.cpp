/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "GaugesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
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
  TAPosition,
  EnableThermalProfile,
  FinalGlideBarDisplayModeControl,
  EnableFinalGlideBarMC0,
  EnableVarioBar
};

static constexpr StaticEnumChoice final_glide_bar_display_mode_list[] = {
  { (unsigned)FinalGlideBarDisplayMode::OFF, N_("Off"),
    N_("Disable final glide bar.") },
  { (unsigned)FinalGlideBarDisplayMode::ON, N_("On"),
    N_("Always show final glide bar.") },
  { (unsigned)FinalGlideBarDisplayMode::AUTO, N_("Auto"),
    N_("Show final glide bar if approaching final glide range.") },
  { 0 }
};

static constexpr StaticEnumChoice thermal_assistant_position_list[] = {
  { (unsigned)UISettings::ThermalAssistantPosition::OFF,
    N_("Off"),
    N_("Disable thermal assistant.") },
  { (unsigned)UISettings::ThermalAssistantPosition::BOTTOM_LEFT,
    N_("Bottom left"),
    N_("Show thermal assistant in bottom left.") },
  { (unsigned)UISettings::ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid infoboxes)"),
    N_("Show thermal assistant in bottom left, above/to right of infoboxes (if there).") },
  { (unsigned)UISettings::ThermalAssistantPosition::BOTTOM_RIGHT,
    N_("Bottom right"),
    N_("Show thermal assistant in bottom right.") },
  { (unsigned)UISettings::ThermalAssistantPosition::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid infoboxes)"),
    N_("Show thermal assistant in bottom right above/to left of infoboxes (if there).") },
  { 0 }
};


class GaugesConfigPanel final : public RowFormWidget, DataFieldListener {
public:
  GaugesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
GaugesConfigPanel::OnModified(DataField &df)
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
                    ui_settings.thermal_assistant_position))
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
