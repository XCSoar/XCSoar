// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceCRendererSettingsPanel.hpp"
#include "Airspace.hpp"
#include "../ColorListDialog.hpp"
#include "ui/canvas/Features.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <cassert>

AirspaceClassRendererSettingsPanel::AirspaceClassRendererSettingsPanel(AirspaceClass _type) noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()), border_color_changed(false),
   fill_color_changed(false), fill_brush_changed(false), type(_type)
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

#ifdef HAVE_HATCHED_BRUSH
#ifdef HAVE_ALPHA_BLEND
  bool transparency = CommonInterface::GetMapSettings().airspace.transparency;
  if (!transparency)
#endif
    AddButton(_("Change Fill Brush"), [this](){
      int pattern_index =
        dlgAirspacePatternsShowModal(UIGlobals::GetLook().map.airspace);

      if (pattern_index >= 0 && pattern_index != settings.brush) {
        settings.brush = pattern_index;
        fill_brush_changed = true;
      }
    });
#ifdef HAVE_ALPHA_BLEND
  else
    AddDummy();
#endif
#else
  AddDummy();
#endif

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

#ifdef HAVE_HATCHED_BRUSH
  if (fill_brush_changed) {
    Profile::SetAirspaceBrush(Profile::map, type, settings.brush);
    changed = true;
  }
#endif

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
