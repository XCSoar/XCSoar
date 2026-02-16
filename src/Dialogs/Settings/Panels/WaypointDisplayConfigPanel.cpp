// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDisplayConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  WaypointLabels,
  WaypointArrivalHeightDisplay,
  WaypointLabelStyle,
  WaypointLabelSelection,
  AppIndLandable,
  AppUseSWLandablesRendering,
  AppLandableRenderingScale,
  AppScaleRunwayLength
};

class WaypointDisplayConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  WaypointDisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void UpdateVisibilities();

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
WaypointDisplayConfigPanel::UpdateVisibilities()
{
  bool visible = GetValueBoolean(AppUseSWLandablesRendering);
  SetRowVisible(AppLandableRenderingScale, visible);
  SetRowVisible(AppScaleRunwayLength, visible);
}

void
WaypointDisplayConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(AppUseSWLandablesRendering, df))
    UpdateVisibilities();
}

void
WaypointDisplayConfigPanel::Prepare(ContainerWindow &parent,
                                    const PixelRect &rc) noexcept
{
  const WaypointRendererSettings &settings = CommonInterface::GetMapSettings().waypoint;

  RowFormWidget::Prepare(parent, rc);

  static constexpr StaticEnumChoice wp_labels_list[] = {
    { WaypointRendererSettings::DisplayTextType::NAME,
      N_("Full name"),
      N_("The full name of each waypoint is displayed.") },
    { WaypointRendererSettings::DisplayTextType::FIRST_WORD,
      N_("First word of name"),
      N_("The first word of the waypoint name is displayed.") },
    { WaypointRendererSettings::DisplayTextType::FIRST_THREE,
      N_("First 3 letters"),
      N_("The first 3 letters of the waypoint name are displayed.") },
    { WaypointRendererSettings::DisplayTextType::FIRST_FIVE,
      N_("First 5 letters"),
      N_("The first 5 letters of the waypoint name are displayed.") },
    { WaypointRendererSettings::DisplayTextType::NONE,
      N_("None"), N_("No waypoint name is displayed.") },
    { WaypointRendererSettings::DisplayTextType::SHORT_NAME,
      N_("Short Name"),
      N_("The short name of each waypoint is displayed. If unavailable, the first five letters of the full name are displayed.") },
    nullptr
  };
  AddEnum(_("Label format"), _("Determines how labels are displayed with each waypoint"),
          wp_labels_list, (unsigned)settings.display_text_type);

  static constexpr StaticEnumChoice wp_arrival_list[] = {
    { WaypointRendererSettings::ArrivalHeightDisplay::NONE,
      N_("None"),
      N_("No arrival height is displayed.") },
    { WaypointRendererSettings::ArrivalHeightDisplay::GLIDE,
      N_("Straight glide"),
      N_("Straight glide arrival height (no terrain is considered).") },
    { WaypointRendererSettings::ArrivalHeightDisplay::TERRAIN,
      N_("Terrain avoidance glide"),
      N_("Arrival height considering terrain avoidance. "
         "Requires \"Reach mode: Turning\" in \"Glide Computer > Route\" settings.") },
    { WaypointRendererSettings::ArrivalHeightDisplay::GLIDE_AND_TERRAIN,
      N_("Straight & terrain glide"),
      N_("Both arrival heights are displayed. "
         "Requires \"Reach mode: Turning\" in \"Glide Computer > Route\" settings.") },
    { WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR,
      N_("Required glide ratio") },
    { WaypointRendererSettings::ArrivalHeightDisplay::REQUIRED_GR_AND_TERRAIN,
      N_("Required GR & terrain glide"),
      N_("Both Required glide ratio and terrain avoidance height are displayed. "
         "Requires \"Reach mode: Turning\" in \"Glide Computer > Route\" settings.") },
    nullptr
  };

  AddEnum(_("Arrival height"), _("Determines how arrival height is displayed in waypoint labels"),
          wp_arrival_list, (unsigned)settings.arrival_height_display);
  SetExpertRow(WaypointArrivalHeightDisplay);

  static constexpr StaticEnumChoice wp_label_list[] = {
    { LabelShape::ROUNDED_BLACK, N_("Rounded rectangle") },
    { LabelShape::OUTLINED_INVERTED, N_("Outlined") },
    nullptr
  };

  AddEnum(_("Label style"), nullptr, wp_label_list,
          (unsigned)settings.landable_render_mode);
  SetExpertRow(WaypointLabelStyle);

  static constexpr StaticEnumChoice wp_selection_list[] = {
    { WaypointRendererSettings::LabelSelection::ALL,
      N_("All"), N_("All labels will be displayed.") },
    { WaypointRendererSettings::LabelSelection::TASK_AND_AIRFIELD,
      N_("Task waypoints & airfields"),
      N_("All waypoints part of a task and all airfields will be displayed.") },
    { WaypointRendererSettings::LabelSelection::TASK_AND_LANDABLE,
      N_("Task waypoints & landables"),
      N_("All waypoints part of a task and all landables will be displayed.") },
    { WaypointRendererSettings::LabelSelection::TASK,
      N_("Task waypoints"),
      N_("All waypoints part of a task will be displayed.") },
    { WaypointRendererSettings::LabelSelection::NONE,
      N_("None"), N_("No labels will be displayed.") },
    nullptr
  };

  AddEnum(_("Label visibility"),
          _("Determines what labels are displayed."),
          wp_selection_list, (unsigned)settings.label_selection);
  SetExpertRow(WaypointLabelSelection);

  static constexpr StaticEnumChoice wp_style_list[] = {
    { WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE,
      N_("Purple circle"),
      N_("Airports and outlanding fields are displayed as purple circles. If the waypoint is "
          "reachable a bigger green circle is added behind the purple one. If the waypoint is "
          "blocked by a mountain the green circle will be red instead.") },
    { WaypointRendererSettings::LandableStyle::BW,
      N_("B/W"),
      N_("Airports and outlanding fields are displayed in white/grey. If the waypoint is "
          "reachable the color is changed to green. If the waypoint is blocked by a mountain "
          "the color is changed to red instead.") },
    { WaypointRendererSettings::LandableStyle::TRAFFIC_LIGHTS,
      N_("Traffic lights"),
      N_("Airports and outlanding fields are displayed in the colors of a traffic light. "
          "Green if reachable, Orange if blocked by mountain and red if not reachable at all.") },
    nullptr
  };
  AddEnum(_("Landable symbols"),
          _("Three styles are available: Purple circles (WinPilot style), a high "
              "contrast (monochrome) style, or orange. The rendering differs for landable "
              "field and airport. All styles mark the waypoints within reach green."),
          wp_style_list, (unsigned)settings.landable_style);

  AddBoolean(_("Detailed landables"),
             _("[Off] Display fixed icons for landables.\n"
                 "[On] Show landables with variable information like runway length and heading."),
             settings.vector_landable_rendering, this);
  SetExpertRow(AppUseSWLandablesRendering);

  AddInteger(_("Landable size"),
             _("A percentage to select the size landables are displayed on the map."),
             "%u %%", "%u", 50, 200, 10, settings.landable_rendering_scale);
  SetExpertRow(AppLandableRenderingScale);

  AddBoolean(_("Scale runway length"),
             _("[Off] Display fixed length for runways.\n"
                 "[On] Scale displayed runway length based on real length."),
             settings.scale_runway_length);
  SetExpertRow(AppScaleRunwayLength);

  UpdateVisibilities();
}

bool
WaypointDisplayConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  WaypointRendererSettings &settings = CommonInterface::SetMapSettings().waypoint;

  changed |= SaveValueEnum(WaypointLabels, ProfileKeys::DisplayText, settings.display_text_type);

  changed |= SaveValueEnum(WaypointArrivalHeightDisplay, ProfileKeys::WaypointArrivalHeightDisplay,
                           settings.arrival_height_display);

  changed |= SaveValueEnum(WaypointLabelStyle, ProfileKeys::WaypointLabelStyle,
                           settings.landable_render_mode);

  changed |= SaveValueEnum(WaypointLabelSelection, ProfileKeys::WaypointLabelSelection,
                           settings.label_selection);

  changed |= SaveValueEnum(AppIndLandable, ProfileKeys::AppIndLandable, settings.landable_style);

  changed |= SaveValue(AppUseSWLandablesRendering, ProfileKeys::AppUseSWLandablesRendering,
                       settings.vector_landable_rendering);

  changed |= SaveValueInteger(AppLandableRenderingScale, ProfileKeys::AppLandableRenderingScale,
                              settings.landable_rendering_scale);

  changed |= SaveValue(AppScaleRunwayLength, ProfileKeys::AppScaleRunwayLength,
                       settings.scale_runway_length);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWaypointDisplayConfigPanel()
{
  return std::make_unique<WaypointDisplayConfigPanel>();
}
