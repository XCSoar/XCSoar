// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapOverlaysConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

static constexpr StaticEnumChoice final_glide_bar_display_mode_list[] = {
  { FinalGlideBarDisplayMode::OFF, N_("Off"),
    N_("Disable final glide bar.") },
  { FinalGlideBarDisplayMode::ON, N_("On"),
    N_("Always show final glide bar.") },
  { FinalGlideBarDisplayMode::AUTO, NC_("Setting", "Auto"),
    N_("Show final glide bar if approaching final glide range.") },
  nullptr
};

/** The bars drawn over the map. */
class MapOverlaysConfigPanel final : public ConfigListPanel {
  bool thermal_profile, mc0, vario_bar;
  FinalGlideBarDisplayMode final_glide;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
MapOverlaysConfigPanel::LoadSettings() noexcept
{
  const MapSettings &map = CommonInterface::GetMapSettings();

  thermal_profile = map.show_thermal_profile;
  final_glide = map.final_glide_bar_display_mode;
  mc0 = map.final_glide_bar_mc0_enabled;
  vario_bar = map.vario_bar_enabled;
}

void
MapOverlaysConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem(_("Thermal Band"),
                _("This enables the display of the thermal profile "
                  "(climb band) display on the map."),
                thermal_profile);

  if (!IsExpert())
    return;

  AddEnumItem(_("Final glide bar"),
              _("If set to \"On\" the final glide will always be shown, "
                "if set to \"Auto\" it will be shown when approaching "
                "the final glide possibility."),
              final_glide_bar_display_mode_list, final_glide);

  if (final_glide != FinalGlideBarDisplayMode::OFF)
    AddToggleItem(_("Final glide bar MC0"),
                  _("If set to \"On\" the final glide bar will show a "
                    "second arrow indicating the required height to reach "
                    "the final waypoint at MC zero."),
                  mc0);

  AddToggleItem(_("Vario bar"),
                _("If set to \"On\" the vario bar will be shown."),
                vario_bar);
}

bool
MapOverlaysConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapSettings &map = CommonInterface::SetMapSettings();

  changed |= Profile::Update(ProfileKeys::EnableThermalProfile,
                             map.show_thermal_profile, thermal_profile);
  changed |= Profile::Update(ProfileKeys::FinalGlideBarDisplayMode,
                             map.final_glide_bar_display_mode, final_glide);
  changed |= Profile::Update(ProfileKeys::EnableFinalGlideBarMC0,
                             map.final_glide_bar_mc0_enabled, mc0);
  changed |= Profile::Update(ProfileKeys::EnableVarioBar,
                             map.vario_bar_enabled, vario_bar);

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateMapOverlaysConfigPanel()
{
  return std::make_unique<MapOverlaysConfigPanel>();
}
