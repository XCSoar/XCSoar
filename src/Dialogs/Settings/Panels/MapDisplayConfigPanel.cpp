// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapDisplayConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Units/Units.hpp"

#include <cmath>

static constexpr StaticEnumChoice orientation_list[] = {
  { MapOrientation::TRACK_UP, N_("Track up"),
    N_("The moving map display will be rotated so the glider's track is oriented up.") },
  { MapOrientation::HEADING_UP, N_("Heading up"),
    N_("The moving map display will be rotated so the glider's heading is oriented up.") },
  { MapOrientation::NORTH_UP, N_("North up"),
    N_("The moving map display will always be orientated north to south and the glider icon will be rotated to show its course.") },
  { MapOrientation::TARGET_UP, N_("Target up"),
    N_("The moving map display will be rotated so the navigation target is oriented up.") },
  { MapOrientation::WIND_UP, N_("Wind up"),
    N_("The moving map display will be rotated so the wind is always oriented up to down. (can be useful for wave flying)") },
  nullptr
};

static constexpr StaticEnumChoice shift_bias_list[] = {
  { MapShiftBias::NONE, N_("None"), N_("Disable adjustments.") },
  { MapShiftBias::TRACK, N_("Track"),
    N_("Use a recent average of the ground track as basis.") },
  { MapShiftBias::TARGET, N_("Target"),
    N_("Use the current target waypoint as basis.") },
  nullptr
};

/* the maximum auto zoom distance, in the unit of the user */
static constexpr int AUTO_ZOOM_DISTANCE_MIN = 20;
static constexpr int AUTO_ZOOM_DISTANCE_MAX = 250;
static constexpr int AUTO_ZOOM_DISTANCE_STEP = 10;

/**
 * Let the user pick the maximum auto zoom distance, one choice per
 * step in the unit of the user.
 *
 * @param value the distance in system units
 * @return true if the value has changed
 */
static bool
PickAutoZoomDistance(const char *caption, const char *help,
                     double &value) noexcept
{
  constexpr unsigned n =
    (AUTO_ZOOM_DISTANCE_MAX - AUTO_ZOOM_DISTANCE_MIN)
    / AUTO_ZOOM_DISTANCE_STEP + 1;

  BasicStringBuffer<char, 32> captions[n];
  PickerChoice choices[n];
  int current = -1;

  const double user_value = Units::ToUserDistance(value);

  for (unsigned i = 0; i < n; ++i) {
    const int distance = AUTO_ZOOM_DISTANCE_MIN + AUTO_ZOOM_DISTANCE_STEP * i;
    captions[i] = FormatUserDistance(Units::ToSysDistance(distance));
    choices[i] = {captions[i].c_str()};

    if (std::fabs(user_value - distance) < AUTO_ZOOM_DISTANCE_STEP / 2.)
      current = i;
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || picked == current)
    return false;

  value = Units::ToSysDistance(AUTO_ZOOM_DISTANCE_MIN
                               + AUTO_ZOOM_DISTANCE_STEP * picked);
  return true;
}

/**
 * The orientation and the zoom of the map.  An item which opens the
 * choice explains itself there; a switch explains itself here, while
 * the cursor is on it.
 */
class MapDisplayConfigPanel final : public ConfigListPanel {
  MapOrientation cruise_orientation, circling_orientation;
  bool circle_zoom;
  MapShiftBias map_shift_bias;
  int glider_screen_position;
  double max_auto_zoom_distance;
  bool distinct_zoom;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
MapDisplayConfigPanel::LoadSettings() noexcept
{
  const MapSettings &settings_map = CommonInterface::GetMapSettings();
  const PageSettings &page_settings = CommonInterface::GetUISettings().pages;

  cruise_orientation = settings_map.cruise_orientation;
  circling_orientation = settings_map.circling_orientation;
  circle_zoom = settings_map.circle_zoom_enabled;
  map_shift_bias = settings_map.map_shift_bias;
  glider_screen_position = settings_map.glider_screen_position;
  max_auto_zoom_distance = settings_map.max_auto_zoom_distance;
  distinct_zoom = page_settings.distinct_zoom;
}

void
MapDisplayConfigPanel::Fill() noexcept
{
  AddGroup(_("Orientation"));

  AddEnumItem(_("Cruise orientation"),
              _("Determines how the screen is rotated with the glider"),
              orientation_list, cruise_orientation);

  AddEnumItem(_("Circling orientation"),
              _("Determines how the screen is rotated with the glider while circling"),
              orientation_list, circling_orientation);

  /* the map is only shifted from the centre while it does not turn
     with the glider */
  if (IsExpert() &&
      (cruise_orientation == MapOrientation::NORTH_UP ||
       cruise_orientation == MapOrientation::WIND_UP))
    AddEnumItem(_("Map shift reference"),
                _("Determines what is used to shift the glider from the map center"),
                shift_bias_list, map_shift_bias);

  if (IsExpert())
    AddPercentItem(_("Glider position offset"),
                   _("Defines the location of the glider drawn on the screen in percent from the screen edge."),
                   10, 50, 5, glider_screen_position);

  AddGroup(_("Zoom"));

  AddToggleItem(_("Circling zoom"),
                _("If enabled, then the map will zoom in automatically when entering circling mode and zoom out automatically when leaving circling mode."),
                circle_zoom);

  if (IsExpert()) {
    static constexpr const char *distance_help =
      N_("The upper limit for auto zoom distance.");

    AddItem(_("Max. auto zoom distance"), [this](){
      if (PickAutoZoomDistance(_("Max. auto zoom distance"),
                               gettext(distance_help),
                               max_auto_zoom_distance))
        Refresh();
    }, {.value = FormatUserDistance(max_auto_zoom_distance).c_str(),
        .chevron = true});

    AddToggleItem(_("Distinct page zoom"),
                  _("Maintain one map zoom level on each page."),
                  distinct_zoom);
  }
}

bool
MapDisplayConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapSettings &settings_map = CommonInterface::SetMapSettings();
  PageSettings &page_settings = CommonInterface::SetUISettings().pages;

  changed |= Profile::Update(ProfileKeys::OrientationCruise,
                             settings_map.cruise_orientation,
                             cruise_orientation);

  changed |= Profile::Update(ProfileKeys::OrientationCircling,
                             settings_map.circling_orientation,
                             circling_orientation);

  changed |= Profile::Update(ProfileKeys::MapShiftBias,
                             settings_map.map_shift_bias, map_shift_bias);

  changed |= Profile::Update(ProfileKeys::GliderScreenPosition,
                             settings_map.glider_screen_position,
                             glider_screen_position);

  changed |= Profile::Update(ProfileKeys::CircleZoom,
                             settings_map.circle_zoom_enabled, circle_zoom);

  changed |= Profile::Update(ProfileKeys::MaxAutoZoomDistance,
                             settings_map.max_auto_zoom_distance,
                             max_auto_zoom_distance);

  changed |= Profile::Update(ProfileKeys::PagesDistinctZoom,
                             page_settings.distinct_zoom, distinct_zoom);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateMapDisplayConfigPanel()
{
  return std::make_unique<MapDisplayConfigPanel>();
}
