// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayoutConfigPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "DisplaySettings.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Dialogs/Settings/SafeAreaStretchWidget.hpp"
#include "Widget/StaticHelpTextWidget.hpp"
#include "Form/DataField/String.hpp"
#include "Form/Edit.hpp"

#include <memory>
#include <string>
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "Menu/ShowButton.hpp"
#include "ActionInterface.hpp"
#include "util/Macros.hpp"

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

enum ControlIndex {
#ifdef HAVE_FULL_SCREEN_SETTING
  FullScreen,
  SafeAreaStretch,
#endif
#ifdef HAVE_STATUS_BAR_SETTING
  StatusBar,
#endif
  MapOrientation,
  DarkMode,
  AppDisplayType,
  AppInfoBoxGeom,
  InfoBoxTitleScale,
  TabDialogStyle,
  AppStatusMessageAlignment,
  AppInfoBoxColors,
  AppInfoBoxTheme,
  AppInfoBoxBorder,
  ShowMenuButton,
  ShowZoomButton,
  ShowQuickMenuButton,
#ifdef DRAW_MOUSE_CURSOR
  CursorSize,
  CursorInverted,
#endif
};

#ifdef HAVE_FULL_SCREEN_SETTING
/**
 * The screen edges the safe area can be stretched out to.
 */
static constexpr struct {
  const char *label;
  DisplaySettings::SafeAreaStretchEdge bit;
} safe_area_stretch_edges[] = {
  { NC_("Screen edge", "Top"), DisplaySettings::SAFE_AREA_STRETCH_TOP },
  { NC_("Screen edge", "Right"), DisplaySettings::SAFE_AREA_STRETCH_RIGHT },
  { NC_("Screen edge", "Bottom"), DisplaySettings::SAFE_AREA_STRETCH_BOTTOM },
  { NC_("Screen edge", "Left"), DisplaySettings::SAFE_AREA_STRETCH_LEFT },
};

/**
 * A one-line hint for the picker dialog; #TwoWidgets would cut a
 * longer paragraph off in landscape.  The full explanation is the
 * help text of the settings row.
 */
static constexpr const char *safe_area_stretch_hint =
  N_("Tap an edge to stretch the safe area out to the screen border "
     "there.");

static constexpr const char *safe_area_stretch_help =
  N_("The safe area is the part of the screen available to the "
     "InfoBoxes, gauges and map overlays. Pick the edges on which it "
     "is stretched out to the screen border. The map always uses the "
     "whole screen, and dialogs and the menu always stay inside the "
     "safe area. Only relevant while full screen mode is enabled.");

/**
 * Describe the enabled edges for the settings list, e.g. "Top, Bottom".
 */
static std::string
FormatSafeAreaStretch(uint8_t edges) noexcept
{
  if (edges == DisplaySettings::SAFE_AREA_STRETCH_NONE)
    return _("None");

  std::string result;

  for (const auto &i : safe_area_stretch_edges) {
    if ((edges & i.bit) == 0)
      continue;

    if (!result.empty())
      result += ", ";
    result += gettext_context("Screen edge", i.label);
  }

  return result;
}

static bool
EditSafeAreaStretch(const char *caption, DataField &df,
                    [[maybe_unused]] const char *help_text)
{
  DisplaySettings &settings = CommonInterface::SetUISettings().display;

  auto picker =
    std::make_unique<SafeAreaStretchWidget>(UIGlobals::GetDialogLook(),
                                            settings.safe_area_stretch);
  auto &picker_ref = *picker;

  TWidgetDialog<StaticHelpTextWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Stretch safe area"));
  dialog.SetWidget(std::move(picker), gettext(safe_area_stretch_hint));
  dialog.AddButton(_("OK"), mrOK);

  /* the row's help text is not reachable from here: WndProperty only
     falls back to OnHelp() when there is no edit callback */
  dialog.AddButton(_("Help"), [caption](){
    HelpDialog(caption, gettext(safe_area_stretch_help));
  });

  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() != mrOK)
    return false;

  const uint8_t edges = picker_ref.GetEdges();
  if (edges == settings.safe_area_stretch)
    return false;

  settings.safe_area_stretch = edges;
  Profile::Set(ProfileKeys::SafeAreaStretch, unsigned(edges));
  Profile::Save();

  /* in "Auto" mode the status bar follows the top edge, and the area
     the InfoBoxes and gauges may use changed */
  CommonInterface::main_window->ApplyFullScreenSettings();
  CommonInterface::main_window->ReinitialiseLayout();

  ((DataFieldString &)df).ModifyValue(FormatSafeAreaStretch(edges).c_str());
  return true;
}

#endif

#ifdef HAVE_STATUS_BAR_SETTING
static constexpr StaticEnumChoice status_bar_list[] = {
  { DisplaySettings::StatusBar::AUTO, NC_("Setting", "Auto"),
    N_("Show the status bar unless the safe area is stretched to the top "
       "screen edge, where it would cover the InfoBoxes.") },
  { DisplaySettings::StatusBar::VISIBLE, N_("Visible"),
    N_("Always show the status bar, even in full screen mode. The "
       "InfoBoxes cannot be drawn behind it, so they keep clear of the "
       "top screen edge.") },
  { DisplaySettings::StatusBar::HIDDEN, N_("Hidden"),
    N_("Never show the status bar.") },
  nullptr
};
#endif

static constexpr StaticEnumChoice display_orientation_list[] = {
  { DisplayOrientation::DEFAULT,
    N_("Default") },
  { DisplayOrientation::PORTRAIT,
    N_("Portrait") },
  { DisplayOrientation::LANDSCAPE,
    N_("Landscape") },
  { DisplayOrientation::REVERSE_PORTRAIT,
    N_("Reverse Portrait") },
  { DisplayOrientation::REVERSE_LANDSCAPE,
    N_("Reverse Landscape") },
  nullptr
};

static constexpr StaticEnumChoice display_type_list[] = {
  { DisplayType::LCD, NC_("Setting", "LCD"),
    N_("Conventional LCD or OLED. Full scrolling animations.") },
  { DisplayType::E_INK, NC_("Setting", "E-ink"),
    N_("Monochrome electronic paper. Disables kinetic and smooth "
       "scrolling.") },
  { DisplayType::COLOR_E_INK, NC_("Setting", "Color e-ink"),
    N_("Color electronic paper. Disables kinetic and smooth "
       "scrolling like monochrome e-ink.") },
  nullptr
};

static_assert(ARRAY_SIZE(display_type_list) ==
              unsigned(DisplayType::COUNT) + 1,
              "display_type_list must match DisplayType::COUNT");

static constexpr StaticEnumChoice info_box_geometry_list[] = {
  { InfoBoxSettings::Geometry::SPLIT_8,
    N_("8 Split") },
  { InfoBoxSettings::Geometry::SPLIT_10,
    N_("10 Split") },
  { InfoBoxSettings::Geometry::SPLIT_3X4,
    N_("12 Split in 3 rows") },
  { InfoBoxSettings::Geometry::SPLIT_3X5,
    N_("15 Split in 3 rows") },
  { InfoBoxSettings::Geometry::SPLIT_3X6,
    N_("18 Split in 3 rows") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_8,
    N_("8 Bottom or Right") },
  { InfoBoxSettings::Geometry::BOTTOM_8_VARIO,
    N_("8 Bottom + Vario (Portrait)") },
  { InfoBoxSettings::Geometry::TOP_LEFT_8,
    N_("8 Top or Left") },
  { InfoBoxSettings::Geometry::TOP_8_VARIO,
    N_("8 Top + Vario (Portrait)") },
  { InfoBoxSettings::Geometry::RIGHT_9_VARIO,
    N_("9 Right + Vario (Landscape)") },
  { InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO,
    N_("9 Left + Right + Vario (Landscape)") },
  { InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO,
    N_("12 Left + 3 Right Vario (Landscape)") },
  { InfoBoxSettings::Geometry::RIGHT_5,
    N_("5 Right (Square)") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_10,
    N_("10 Bottom or Right") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_12,
    N_("12 Bottom or Right") },
  { InfoBoxSettings::Geometry::TOP_LEFT_10,
    N_("10 Top or Left") },
  { InfoBoxSettings::Geometry::TOP_LEFT_12,
    N_("12 Top or Left") },
  { InfoBoxSettings::Geometry::RIGHT_16,
    N_("16 Right (Landscape)") },
  { InfoBoxSettings::Geometry::RIGHT_24,
    N_("24 Bottom or Right") },
  { InfoBoxSettings::Geometry::TOP_LEFT_4,
    N_("4 Top or Left") },
  { InfoBoxSettings::Geometry::BOTTOM_RIGHT_4,
    N_("4 Bottom or Right") },
  nullptr
};

static constexpr StaticEnumChoice tabdialog_style_list[] = {
  { DialogSettings::TabStyle::Text, N_("Text"),
    N_("Show text on tabbed dialogs.") },
  { DialogSettings::TabStyle::Icon, N_("Icons"),
    N_("Show icons on tabbed dialogs.")},
  nullptr
};

static constexpr StaticEnumChoice popup_msg_position_list[] = {
  { UISettings::PopupMessagePosition::CENTER, N_("Center"),
    N_("Center the status message boxes.") },
  { UISettings::PopupMessagePosition::TOP_LEFT, N_("Top left"),
    N_("Show status message boxes in the top left corner.") },
  nullptr
};

static constexpr StaticEnumChoice infobox_border_list[] = {
  { InfoBoxSettings::BorderStyle::BOX,
    N_("Box"), N_("Draws boxes around each InfoBox.") },
  { InfoBoxSettings::BorderStyle::TAB,
    N_("Tab"), N_("Draws a tab at the top of the InfoBox across the title.") },
  { InfoBoxSettings::BorderStyle::SHADED,
    N_("Shaded"), nullptr /* TODO: help text */ },
  { InfoBoxSettings::BorderStyle::GLASS,
    N_("Glass"), nullptr /* TODO: help text */ },
  nullptr
};

static constexpr StaticEnumChoice dark_mode_list[] = {
  { UISettings::DarkMode::AUTO, NC_("Setting", "Auto"),
    N_("Use the system-wide setting") },
  { UISettings::DarkMode::OFF, N_("Off"),
    N_("Black text on white background") },
  { UISettings::DarkMode::ON, N_("On"),
    N_("White text on black background") },
  nullptr
};

static constexpr StaticEnumChoice infobox_theme_list[] = {
  { InfoBoxSettings::Theme::FOLLOW_GLOBAL, N_("Follow global"),
    N_("Use the same light/dark mode as the overall UI.") },
  { InfoBoxSettings::Theme::LIGHT, N_("Light"),
    N_("Always use dark text on a light InfoBox background.") },
  { InfoBoxSettings::Theme::DARK, N_("Dark"),
    N_("Always use light text on a dark InfoBox background.") },
  nullptr
};

class LayoutConfigPanel final : public RowFormWidget {
  /** Geometry when this panel was opened; restored if Settings is cancelled. */
  InfoBoxSettings::Geometry original_geometry{};
  bool saved = false;

public:
  LayoutConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Leave() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
LayoutConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  original_geometry = ui_settings.info_boxes.geometry;
  saved = false;

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_FULL_SCREEN_SETTING
  AddBoolean(_("Full screen"),
             _("Let XCSoar use the whole screen, including the areas behind "
               "the system bars and the display cutout. The map fills it "
               "completely, while \"Stretch safe area\" decides which edges "
               "the InfoBoxes, gauges and map overlays may reach."),
             ui_settings.display.full_screen);

  auto *edges_row =
    Add(_("Stretch safe area"), gettext(safe_area_stretch_help),
        new DataFieldString(FormatSafeAreaStretch(ui_settings.display
                                                  .safe_area_stretch).c_str()));
  edges_row->SetEditCallback(EditSafeAreaStretch);
  SetExpertRow(SafeAreaStretch);
#endif

#ifdef HAVE_STATUS_BAR_SETTING
  AddEnum(_("System status bar"),
          _("Whether the system status bar with the clock and the battery "
            "level stays visible."),
          status_bar_list,
          unsigned(ui_settings.display.status_bar));
#endif

  if (Display::RotateSupported())
    AddEnum(_("Display orientation"), _("Rotate the display on devices that support it."),
            display_orientation_list, (unsigned)ui_settings.display.orientation);
  else
    AddDummy();

#ifndef KOBO
  AddEnum(_("Dark mode"), nullptr, dark_mode_list,
          (unsigned)ui_settings.dark_mode);
  SetExpertRow(DarkMode);
#else
  AddDummy();
#endif

  AddEnum(C_("Setting", "Display type"),
          _("Select the display technology. E-ink modes disable kinetic "
            "and smooth scrolling for slow refresh screens."),
          display_type_list,
          (unsigned)ui_settings.display.display_type);
  SetExpertRow(AppDisplayType);

  AddEnum(_("InfoBox geometry"),
          _("A list of possible InfoBox layouts. Do some trials to find the best for your screen size."),
          info_box_geometry_list, (unsigned)ui_settings.info_boxes.geometry);

  AddInteger(_("InfoBox title size"), _("Zoom factor for InfoBox title and comment text"),
             "%d %%", "%d", 50, 150, 5,
             ui_settings.info_boxes.scale_title_font);
  SetExpertRow(InfoBoxTitleScale);

  AddEnum(_("Tab dialog style"), nullptr,
          tabdialog_style_list, (unsigned)ui_settings.dialog.tab_style);

  AddEnum(_("Message display"), nullptr,
          popup_msg_position_list,
          (unsigned)ui_settings.popup_message_position);
  SetExpertRow(AppStatusMessageAlignment);

  if (HasColors()) {
    AddBoolean(_("Colored InfoBoxes"),
               _("If true, certain InfoBoxes will have coloured text. For example, the active waypoint "
                 "InfoBox will be blue when the glider is above final glide."),
               ui_settings.info_boxes.use_colors);
    SetExpertRow(AppInfoBoxColors);
  } else
    AddDummy();

  AddEnum(_("InfoBox theme"), nullptr, infobox_theme_list,
          (unsigned)ui_settings.info_boxes.theme);
  SetExpertRow(AppInfoBoxTheme);

  AddEnum(_("InfoBox border"), nullptr, infobox_border_list,
          unsigned(ui_settings.info_boxes.border_style));
  SetExpertRow(AppInfoBoxBorder);

  AddBoolean(_("Show Menu button"), _("Show the Menu button"),
             ui_settings.show_menu_button);
  SetExpertRow(ShowMenuButton);
  AddBoolean(_("Show Zoom button"), _("Show the Zoom button"),
             ui_settings.show_zoom_button);
  SetExpertRow(ShowZoomButton);
  AddBoolean(C_("Setting", "Show QuickMenu button"),
             _("Show the QuickMenu button"),
             ui_settings.show_quickmenu_button);
  SetExpertRow(ShowQuickMenuButton);

#ifdef DRAW_MOUSE_CURSOR
  AddInteger(_("Cursor zoom"), _("Cursor zoom factor"), "%d x", "%d x", 1, 10, 1,
             (unsigned)ui_settings.display.cursor_size);
  AddBoolean(_("Invert cursor color"), _("Enable black cursor"),
             ui_settings.display.invert_cursor_colors);
#endif
}

void
LayoutConfigPanel::Unprepare() noexcept
{
  if (!saved)
    CommonInterface::SetUISettings().info_boxes.geometry = original_geometry;

  RowFormWidget::Unprepare();
}

bool
LayoutConfigPanel::Leave() noexcept
{
  /* Switching to another settings page (still inside Configuration):
     copy geometry so InfoBox Sets can read settings.geometry. */
  SaveValueEnum(AppInfoBoxGeom,
                CommonInterface::SetUISettings().info_boxes.geometry);
  return true;
}

bool
LayoutConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();
  saved = true;

#ifdef HAVE_FULL_SCREEN_SETTING
  bool full_screen_changed =
    SaveValue(FullScreen, ProfileKeys::FullScreen,
              ui_settings.display.full_screen);
  /* the per-edge settings are applied by their own dialog */
  changed |= full_screen_changed;

#ifdef HAVE_STATUS_BAR_SETTING
  if (SaveValueEnum(StatusBar, ProfileKeys::StatusBar,
                    ui_settings.display.status_bar)) {
    changed = true;
    full_screen_changed = true;
  }
#endif
#endif

  bool orientation_changed = false;

  if (Display::RotateSupported()) {
    orientation_changed =
      SaveValueEnum(MapOrientation, ProfileKeys::MapOrientation,
                    ui_settings.display.orientation);
    changed |= orientation_changed;
  }

#ifndef KOBO
  changed |= SaveValueEnum(DarkMode, ProfileKeys::DarkMode,
                           ui_settings.dark_mode);
#else
  if (ui_settings.dark_mode != UISettings::DarkMode::OFF) {
    ui_settings.dark_mode = UISettings::DarkMode::OFF;
    changed = true;
  }
#endif

  if (SaveValueEnum(AppDisplayType, ProfileKeys::DisplayType,
                    ui_settings.display.display_type)) {
    changed = true;
    SetDisplayType(ui_settings.display.display_type);
  }

  bool info_box_geometry_changed = false;

  /* Leave() may already have synced the DataField into ui_settings;
     re-base so SaveValueEnum still writes the profile when needed. */
  ui_settings.info_boxes.geometry = original_geometry;
  info_box_geometry_changed |=
    SaveValueEnum(AppInfoBoxGeom, ProfileKeys::InfoBoxGeometry,
                  ui_settings.info_boxes.geometry);
  info_box_geometry_changed |=
    SaveValueInteger(InfoBoxTitleScale, ProfileKeys::InfoBoxTitleScale,
                  ui_settings.info_boxes.scale_title_font);

  changed |= info_box_geometry_changed;

  changed |= SaveValueEnum(AppStatusMessageAlignment, ProfileKeys::AppStatusMessageAlignment,
                           ui_settings.popup_message_position);

  if (HasColors())
    changed |= SaveValue(AppInfoBoxColors, ProfileKeys::AppInfoBoxColors,
                         ui_settings.info_boxes.use_colors);

  changed |= SaveValueEnum(AppInfoBoxTheme, ProfileKeys::AppInfoBoxTheme,
                           ui_settings.info_boxes.theme);

  changed |= SaveValueEnum(AppInfoBoxBorder, ProfileKeys::AppInfoBoxBorder,
                           ui_settings.info_boxes.border_style);

  bool overlay_buttons_changed = false;
  if (SaveValue(ShowMenuButton, ProfileKeys::ShowMenuButton,
                ui_settings.show_menu_button))
    overlay_buttons_changed = changed = true;
  if (SaveValue(ShowZoomButton, ProfileKeys::ShowZoomButton,
                ui_settings.show_zoom_button))
    overlay_buttons_changed = changed = true;
  if (SaveValue(ShowQuickMenuButton, ProfileKeys::ShowQuickMenuButton,
                ui_settings.show_quickmenu_button))
    overlay_buttons_changed = changed = true;
  if (overlay_buttons_changed)
    CommonInterface::main_window->ReinitialiseMapOverlayButtons();

  DialogSettings &dialog_settings = CommonInterface::SetUISettings().dialog;
  changed |= SaveValueEnum(TabDialogStyle, ProfileKeys::AppDialogTabStyle, dialog_settings.tab_style);

#ifdef DRAW_MOUSE_CURSOR
  changed |= SaveValueInteger(CursorSize, ProfileKeys::CursorSize,
                              ui_settings.display.cursor_size);
  CommonInterface::main_window->SetCursorSize(ui_settings.display.cursor_size);

  changed |= SaveValue(CursorInverted, ProfileKeys::CursorColorsInverted, ui_settings.display.invert_cursor_colors);
  CommonInterface::main_window->SetCursorColorsInverted(ui_settings.display.invert_cursor_colors);
#endif

#ifdef HAVE_FULL_SCREEN_SETTING
  /* this may change the usable screen area, so do it late, when the
     remaining settings have been read from the form */
  if (full_screen_changed)
    CommonInterface::main_window->ApplyFullScreenSettings();
#endif

  if (orientation_changed) {
    assert(Display::RotateSupported());

    if (!Display::Rotate(ui_settings.display.orientation))
      LogString("Display rotation failed");

#ifdef USE_POLL_EVENT
    UI::event_queue->SetDisplayOrientation(ui_settings.display.orientation);
#endif

    CommonInterface::main_window->CheckResize();
  } else if (info_box_geometry_changed)
    CommonInterface::main_window->ReinitialiseLayout();

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateLayoutConfigPanel()
{
  return std::make_unique<LayoutConfigPanel>();
}
