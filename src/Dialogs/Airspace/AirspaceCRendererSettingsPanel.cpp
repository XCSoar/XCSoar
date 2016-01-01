/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "AirspaceCRendererSettingsPanel.hpp"
#include "Airspace.hpp"
#include "../ColorListDialog.hpp"
#include "Screen/Features.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <assert.h>

AirspaceClassRendererSettingsPanel::AirspaceClassRendererSettingsPanel(AirspaceClass _type)
  :RowFormWidget(UIGlobals::GetDialogLook()), border_color_changed(false),
   fill_color_changed(false), fill_brush_changed(false), type(_type)
{
  assert(type < AIRSPACECLASSCOUNT);
}

void
AirspaceClassRendererSettingsPanel::OnAction(int id)
{
  if (id == BorderColor)
    border_color_changed |= ShowColorListDialog(settings.border_color);

  if (id == FillColor)
    fill_color_changed |= ShowColorListDialog(settings.fill_color);

#ifdef HAVE_HATCHED_BRUSH
  if (id == FillBrush) {
    int pattern_index =
      dlgAirspacePatternsShowModal(UIGlobals::GetLook().map.airspace);

    if (pattern_index >= 0 && pattern_index != settings.brush) {
      settings.brush = pattern_index;
      fill_brush_changed = true;
    }
  }
#endif
}

void
AirspaceClassRendererSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  // Save a working copy
  settings = CommonInterface::GetMapSettings().airspace.classes[type];

  // Add controls
  AddButton(_("Change Border Color"), *this, BorderColor);
  AddButton(_("Change Fill Color"), *this, FillColor);

#ifdef HAVE_HATCHED_BRUSH
#ifdef HAVE_ALPHA_BLEND
  bool transparency = CommonInterface::GetMapSettings().airspace.transparency;
  if (!transparency)
#endif
    AddButton(_("Change Fill Brush"), *this, FillBrush);
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
             _T("%d"), _T("%d"), 0, 5, 1, settings.border_width);

  static constexpr StaticEnumChoice fill_mode_list[] = {
    { (unsigned)AirspaceClassRendererSettings::FillMode::ALL, N_("Filled"), },
    { (unsigned)AirspaceClassRendererSettings::FillMode::PADDING, N_("Only padding"), },
    { (unsigned)AirspaceClassRendererSettings::FillMode::NONE, N_("Not filled"), },
    { 0 }
  };

  AddEnum(_("Fill Mode"),
          _("Defines how the airspace is filled with the configured color."),
          fill_mode_list, (unsigned)settings.fill_mode);
}

bool
AirspaceClassRendererSettingsPanel::Save(bool &changed)
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

  const auto &border_width_df = GetDataField(BorderWidth);
  unsigned border_width = border_width_df.GetAsInteger();
  if (border_width != settings.border_width) {
    settings.border_width = border_width;
    Profile::SetAirspaceBorderWidth(Profile::map, type, border_width);
    changed = true;
  }

  const auto &fill_mode_df = GetDataField(FillMode);
  auto fill_mode = (AirspaceClassRendererSettings::FillMode)fill_mode_df.GetAsInteger();
  if (fill_mode != settings.fill_mode) {
    settings.fill_mode = fill_mode;
    Profile::SetAirspaceFillMode(Profile::map, type, (unsigned)fill_mode);
    changed = true;
  }

  if (changed)
    CommonInterface::SetMapSettings().airspace.classes[type] = settings;

  return true;
}
