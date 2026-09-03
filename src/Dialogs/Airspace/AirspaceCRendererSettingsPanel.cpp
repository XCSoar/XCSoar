// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceCRendererSettingsPanel.hpp"
#include "../ColorListDialog.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

#include <cassert>

AirspaceClassRendererSettingsPanel::AirspaceClassRendererSettingsPanel(AirspaceClass _type) noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()), border_color_changed(false),
   fill_color_changed(false), type(_type)
{
  assert(type < AIRSPACECLASSCOUNT);
}

void
AirspaceClassRendererSettingsPanel::Prepare(ContainerWindow &parent,
                                            const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  // Save a working copy
  settings = CommonInterface::GetMapSettings().airspace.classes[type];

  // Add controls
  AddButton(_("Change Border Color"), [this](){
    border_color_changed |= ShowColorListDialog(settings.border_color);
  });

  AddButton(_("Change Fill Color"), [this](){
    fill_color_changed |= ShowColorListDialog(settings.fill_color);
  });

  AddInteger(_("Border Width"),
             _("The width of the border drawn around each airspace. "
               "Set this value to zero to hide the border."),
             "%d", "%d", 0, 5, 1, settings.border_width);

  static constexpr StaticEnumChoice fill_mode_list[] = {
    { AirspaceClassRendererSettings::FillMode::ALL, N_("Filled"), },
    { AirspaceClassRendererSettings::FillMode::PADDING, N_("Only padding"), },
    { AirspaceClassRendererSettings::FillMode::NONE, N_("Not filled"), },
    nullptr
  };

  AddEnum(_("Fill Mode"),
          _("Defines how the airspace is filled with the configured color."),
          fill_mode_list, (unsigned)settings.fill_mode);
}

bool
AirspaceClassRendererSettingsPanel::Save(bool &changed) noexcept
{
  if (border_color_changed) {
    Profile::SetAirspaceBorderColor(Profile::map, type, settings.border_color);
    changed = true;
  }

  if (fill_color_changed) {
    Profile::SetAirspaceFillColor(Profile::map, type, settings.fill_color);
    changed = true;
  }

  if (SaveValueInteger(BorderWidth, settings.border_width)) {
    Profile::SetAirspaceBorderWidth(Profile::map, type, settings.border_width);
    changed = true;
  }

  if (SaveValueEnum(FillMode, settings.fill_mode)) {
    Profile::SetAirspaceFillMode(Profile::map, type,
                                 (unsigned)settings.fill_mode);
    changed = true;
  }

  if (changed)
    CommonInterface::SetMapSettings().airspace.classes[type] = settings;

  return true;
}
