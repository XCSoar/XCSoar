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

#include "MapDisplayConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  OrientationCruise,
  OrientationCircling,
  CirclingZoom,
  MAP_SHIFT_BIAS,
  GliderScreenPosition,
  MaxAutoZoomDistance,
  PAGES_DISTINCT_ZOOM,
};

static constexpr StaticEnumChoice orientation_list[] = {
  { (unsigned)MapOrientation::TRACK_UP, N_("Track up"),
    N_("The moving map display will be rotated so the glider's track is oriented up.") },
  { (unsigned)MapOrientation::HEADING_UP, N_("Heading up"),
    N_("The moving map display will be rotated so the glider's heading is oriented up.") },
  { (unsigned)MapOrientation::NORTH_UP, N_("North up"),
    N_("The moving map display will always be orientated north to south and the glider icon will be rotated to show its course.") },
  { (unsigned)MapOrientation::TARGET_UP, N_("Target up"),
    N_("The moving map display will be rotated so the navigation target is oriented up.") },
  { (unsigned)MapOrientation::WIND_UP, N_("Wind up"),
    N_("The moving map display will be rotated so the wind is always oriented up to down. (can be useful for wave flying)") },
  { 0 }
};

static constexpr StaticEnumChoice shift_bias_list[] = {
  { (unsigned)MapShiftBias::NONE, N_("None"), N_("Disable adjustments.") },
  { (unsigned)MapShiftBias::TRACK, N_("Track"),
    N_("Use a recent average of the ground track as basis.") },
  { (unsigned)MapShiftBias::TARGET, N_("Target"),
    N_("Use the current target waypoint as basis.") },
  { 0 }
};

class MapDisplayConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  MapDisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void UpdateVisibilities();

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
MapDisplayConfigPanel::UpdateVisibilities()
{
  auto orientation = (MapOrientation)GetValueInteger(OrientationCruise);

  SetRowVisible(MAP_SHIFT_BIAS,
                orientation == MapOrientation::NORTH_UP ||
                orientation == MapOrientation::WIND_UP);
}

void
MapDisplayConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(OrientationCruise, df) ||
      IsDataField(OrientationCircling, df) ||
      IsDataField(MAP_SHIFT_BIAS, df)) {
    UpdateVisibilities();
  }
}

void
MapDisplayConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const MapSettings &settings_map = CommonInterface::GetMapSettings();
  const PageSettings &page_settings = CommonInterface::GetUISettings().pages;

  AddEnum(_("Cruise orientation"),
          _("Determines how the screen is rotated with the glider"),
          orientation_list,
          (unsigned)settings_map.cruise_orientation,
          this);

  AddEnum(_("Circling orientation"),
          _("Determines how the screen is rotated with the glider while circling"),
          orientation_list,
          (unsigned)settings_map.circling_orientation,
          this);

  AddBoolean(_("Circling zoom"),
             _("If enabled, then the map will zoom in automatically when entering circling mode and zoom out automatically when leaving circling mode."),
             settings_map.circle_zoom_enabled);

  AddEnum(_("Map shift reference"),
          _("Determines what is used to shift the glider from the map center"),
          shift_bias_list,
          (unsigned)settings_map.map_shift_bias,
          this);
  SetExpertRow(MAP_SHIFT_BIAS);

  AddInteger(_("Glider position offset"),
             _("Defines the location of the glider drawn on the screen in percent from the screen edge."),
             _T("%d %%"), _T("%d"), 10, 50, 5,
             settings_map.glider_screen_position);
  SetExpertRow(GliderScreenPosition);

  AddFloat(_("Max. auto zoom distance"),
           _("The upper limit for auto zoom distance."),
           _T("%.0f %s"), _T("%.0f"), 20, 250, 10, false,
           UnitGroup::DISTANCE, settings_map.max_auto_zoom_distance);
  SetExpertRow(MaxAutoZoomDistance);

  AddBoolean(_("Distinct page zoom"),
             _("Maintain one map zoom level on each page."),
             page_settings.distinct_zoom);
  SetExpertRow(PAGES_DISTINCT_ZOOM);

  UpdateVisibilities();
}

bool
MapDisplayConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  MapSettings &settings_map = CommonInterface::SetMapSettings();
  PageSettings &page_settings = CommonInterface::SetUISettings().pages;

  changed |= SaveValueEnum(OrientationCruise, ProfileKeys::OrientationCruise,
                           settings_map.cruise_orientation);

  changed |= SaveValueEnum(OrientationCircling, ProfileKeys::OrientationCircling,
                           settings_map.circling_orientation);

  changed |= SaveValueEnum(MAP_SHIFT_BIAS, ProfileKeys::MapShiftBias,
                           settings_map.map_shift_bias);

  changed |= SaveValue(GliderScreenPosition, ProfileKeys::GliderScreenPosition,
                       settings_map.glider_screen_position);

  changed |= SaveValue(CirclingZoom, ProfileKeys::CircleZoom,
                       settings_map.circle_zoom_enabled);

  changed |= SaveValue(MaxAutoZoomDistance, UnitGroup::DISTANCE,
                       ProfileKeys::MaxAutoZoomDistance,
                       settings_map.max_auto_zoom_distance);

  changed |= SaveValue(PAGES_DISTINCT_ZOOM, ProfileKeys::PagesDistinctZoom,
                       page_settings.distinct_zoom);

  _changed |= changed;

  return true;
}

Widget *
CreateMapDisplayConfigPanel()
{
  return new MapDisplayConfigPanel();
}
