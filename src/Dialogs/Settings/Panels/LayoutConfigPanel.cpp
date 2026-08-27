// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayoutConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "Menu/ShowButton.hpp"

enum ControlIndex {
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
};

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
