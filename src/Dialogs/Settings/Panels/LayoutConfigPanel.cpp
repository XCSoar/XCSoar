// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayoutConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "InfoBoxes/InfoBoxGeometryList.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Language/Language.hpp"
#include "Asset.hpp"

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

static constexpr StaticEnumChoice infobox_theme_list[] = {
  { InfoBoxSettings::Theme::FOLLOW_GLOBAL, N_("Follow global"),
    N_("Use the same light/dark mode as the overall UI.") },
  { InfoBoxSettings::Theme::LIGHT, N_("Light"),
    N_("Always use dark text on a light InfoBox background.") },
  { InfoBoxSettings::Theme::DARK, N_("Dark"),
    N_("Always use light text on a dark InfoBox background.") },
  nullptr
};

/**
 * Where the InfoBoxes stand and how they look, the style of the
 * dialogs and the messages, and the buttons on the map.
 */
class LayoutConfigPanel final : public ConfigListPanel {
  /** the geometry when the page was opened; restored if it is cancelled */
  InfoBoxSettings::Geometry original_geometry{};
  bool saved = false;

  InfoBoxSettings::Geometry geometry;
  int scale_title_font;
  bool use_colors;
  InfoBoxSettings::Theme theme;
  InfoBoxSettings::BorderStyle border_style;

  DialogSettings::TabStyle tab_style;
  UISettings::PopupMessagePosition popup_message_position;

  bool show_menu_button, show_zoom_button, show_quickmenu_button;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  void Unprepare() noexcept override;
  bool Leave() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
LayoutConfigPanel::LoadSettings() noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  original_geometry = ui_settings.info_boxes.geometry;
  saved = false;

  geometry = ui_settings.info_boxes.geometry;
  scale_title_font = ui_settings.info_boxes.scale_title_font;
  use_colors = ui_settings.info_boxes.use_colors;
  theme = ui_settings.info_boxes.theme;
  border_style = ui_settings.info_boxes.border_style;

  tab_style = ui_settings.dialog.tab_style;
  popup_message_position = ui_settings.popup_message_position;

  show_menu_button = ui_settings.show_menu_button;
  show_zoom_button = ui_settings.show_zoom_button;
  show_quickmenu_button = ui_settings.show_quickmenu_button;
}

void
LayoutConfigPanel::Fill() noexcept
{
  AddGroup(_("InfoBoxes"));

  AddEnumItem(_("InfoBox geometry"),
              _("A list of possible InfoBox layouts. Do some trials to find the best for your screen size."),
              info_box_geometry_list, geometry);

  if (IsExpert()) {
    AddPercentItem(_("InfoBox title size"),
                   _("Zoom factor for InfoBox title and comment text"),
                   50, 150, 5, scale_title_font);

    if (HasColors())
      AddToggleItem(_("Colored InfoBoxes"),
                    _("If true, certain InfoBoxes will have coloured text. For example, the active waypoint "
                      "InfoBox will be blue when the glider is above final glide."),
                    use_colors);

    AddEnumItem(_("InfoBox theme"), nullptr, infobox_theme_list, theme);

    AddEnumItem(_("InfoBox border"), nullptr, infobox_border_list,
                border_style);
  }

  /* the dialogs and the messages */
  AddGroup();

  AddEnumItem(_("Tab dialog style"), nullptr, tabdialog_style_list,
              tab_style);

  if (IsExpert())
    AddEnumItem(_("Message display"), nullptr, popup_msg_position_list,
                popup_message_position);

  /* the buttons on the map */
  if (IsExpert()) {
    AddGroup();

    AddToggleItem(_("Show Menu button"), _("Show the Menu button"),
                  show_menu_button);
    AddToggleItem(_("Show Zoom button"), _("Show the Zoom button"),
                  show_zoom_button);
    AddToggleItem(C_("Setting", "Show QuickMenu button"),
                  _("Show the QuickMenu button"),
                  show_quickmenu_button);
  }
}

void
LayoutConfigPanel::Unprepare() noexcept
{
  if (!saved)
    CommonInterface::SetUISettings().info_boxes.geometry = original_geometry;

  ConfigListPanel::Unprepare();
}

bool
LayoutConfigPanel::Leave() noexcept
{
  /* Switching to another settings page (still inside Configuration):
     copy geometry so InfoBox Sets can read settings.geometry. */
  CommonInterface::SetUISettings().info_boxes.geometry = geometry;
  return ConfigListPanel::Leave();
}

bool
LayoutConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();
  saved = true;

  /* Leave() may already have copied the geometry into ui_settings;
     re-base so Update() still writes the profile when needed. */
  ui_settings.info_boxes.geometry = original_geometry;

  const bool geometry_changed =
    Profile::Update(ProfileKeys::InfoBoxGeometry,
                    ui_settings.info_boxes.geometry, geometry);
  const bool title_scale_changed =
    Profile::Update(ProfileKeys::InfoBoxTitleScale,
                    ui_settings.info_boxes.scale_title_font,
                    unsigned(scale_title_font));
  const bool info_box_geometry_changed =
    geometry_changed || title_scale_changed;

  changed |= info_box_geometry_changed;

  changed |= Profile::Update(ProfileKeys::AppStatusMessageAlignment,
                             ui_settings.popup_message_position,
                             popup_message_position);

  if (HasColors())
    changed |= Profile::Update(ProfileKeys::AppInfoBoxColors,
                               ui_settings.info_boxes.use_colors,
                               use_colors);

  changed |= Profile::Update(ProfileKeys::AppInfoBoxTheme,
                             ui_settings.info_boxes.theme, theme);

  changed |= Profile::Update(ProfileKeys::AppInfoBoxBorder,
                             ui_settings.info_boxes.border_style,
                             border_style);

  const bool menu_button_changed =
    Profile::Update(ProfileKeys::ShowMenuButton,
                    ui_settings.show_menu_button, show_menu_button);
  const bool zoom_button_changed =
    Profile::Update(ProfileKeys::ShowZoomButton,
                    ui_settings.show_zoom_button, show_zoom_button);
  const bool quickmenu_button_changed =
    Profile::Update(ProfileKeys::ShowQuickMenuButton,
                    ui_settings.show_quickmenu_button,
                    show_quickmenu_button);
  if (menu_button_changed || zoom_button_changed ||
      quickmenu_button_changed) {
    changed = true;
    CommonInterface::main_window->ReinitialiseMapOverlayButtons();
  }

  changed |= Profile::Update(ProfileKeys::AppDialogTabStyle,
                             ui_settings.dialog.tab_style, tab_style);

  if (info_box_geometry_changed)
    CommonInterface::main_window->ReinitialiseLayout();

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateLayoutConfigPanel()
{
  return std::make_unique<LayoutConfigPanel>();
}
