// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapOverlaysConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  EnableThermalProfile,
  FinalGlideBarDisplayModeControl,
  EnableFinalGlideBarMC0,
  EnableVarioBar,
};

static constexpr StaticEnumChoice final_glide_bar_display_mode_list[] = {
  { FinalGlideBarDisplayMode::OFF, N_("Off"),
    N_("Disable final glide bar.") },
  { FinalGlideBarDisplayMode::ON, N_("On"),
    N_("Always show final glide bar.") },
  { FinalGlideBarDisplayMode::AUTO, NC_("Setting", "Auto"),
    N_("Show final glide bar if approaching final glide range.") },
  nullptr
};

class MapOverlaysConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  MapOverlaysConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
MapOverlaysConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(FinalGlideBarDisplayModeControl, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    FinalGlideBarDisplayMode fgbdm = (FinalGlideBarDisplayMode)dfe.GetValue();
    SetRowAvailable(EnableFinalGlideBarMC0,
                    fgbdm != FinalGlideBarDisplayMode::OFF);
  }
}

void
MapOverlaysConfigPanel::Prepare(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Thermal Band"),
             _("This enables the display of the thermal profile "
               "(climb band) display on the map."),
             map_settings.show_thermal_profile);

  AddEnum(_("Final glide bar"),
          _("If set to \"On\" the final glide will always be shown, "
            "if set to \"Auto\" it will be shown when approaching "
            "the final glide possibility."),
          final_glide_bar_display_mode_list,
          (unsigned)map_settings.final_glide_bar_display_mode,
          this);
  SetExpertRow(FinalGlideBarDisplayModeControl);

  AddBoolean(_("Final glide bar MC0"),
             _("If set to \"On\" the final glide bar will show a "
               "second arrow indicating the required height to reach "
               "the final waypoint at MC zero."),
             map_settings.final_glide_bar_mc0_enabled);
  SetExpertRow(EnableFinalGlideBarMC0);

  SetRowAvailable(EnableFinalGlideBarMC0,
                  map_settings.final_glide_bar_display_mode !=
                    FinalGlideBarDisplayMode::OFF);

  AddBoolean(_("Vario bar"),
             _("If set to \"On\" the vario bar will be shown."),
             map_settings.vario_bar_enabled);
  SetExpertRow(EnableVarioBar);
}

bool
MapOverlaysConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapSettings &map_settings = CommonInterface::SetMapSettings();

  changed |= SaveValue(EnableThermalProfile, ProfileKeys::EnableThermalProfile,
                       map_settings.show_thermal_profile);

  changed |= SaveValueEnum(FinalGlideBarDisplayModeControl,
                           ProfileKeys::FinalGlideBarDisplayMode,
                           map_settings.final_glide_bar_display_mode);

  changed |= SaveValue(EnableFinalGlideBarMC0,
                       ProfileKeys::EnableFinalGlideBarMC0,
                       map_settings.final_glide_bar_mc0_enabled);

  changed |= SaveValue(EnableVarioBar, ProfileKeys::EnableVarioBar,
                       map_settings.vario_bar_enabled);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateMapOverlaysConfigPanel()
{
  return std::make_unique<MapOverlaysConfigPanel>();
}
