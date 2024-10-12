// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayoutConfigPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Asset.hpp"
#include "Menu/ShowMenuButton.hpp"
#include "ActionInterface.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

enum ControlIndex {
#ifdef ANDROID
  FullScreen,
#endif
  MapOrientation,
  DarkMode,
  AppInfoBoxGeom,
  InfoBoxTitleScale,
  TabDialogStyle,
  AppStatusMessageAlignment,
  AppInfoBoxColors,
  AppInfoBoxBorder,
#ifdef KOBO
  ShowMenuButton,
#endif
#ifdef DRAW_MOUSE_CURSOR
  CursorSize,
  CursorInverted,
#endif
};

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
  { UISettings::PopupMessagePosition::TOP_LEFT, N_("Topleft"),
    N_("Show status message boxes ina the top left corner.") },
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
  { UISettings::DarkMode::AUTO, N_("Auto"),
    N_("Use the system-wide setting") },
  { UISettings::DarkMode::OFF, N_("Off"),
    N_("Black text on white background") },
  { UISettings::DarkMode::ON, N_("On"),
    N_("White text on black background") },
  nullptr
};

class LayoutConfigPanel final : public RowFormWidget {
public:
  LayoutConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
LayoutConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

#ifdef ANDROID
  AddBoolean(_("Full screen"), _("Run XCSoar in full screen mode"),
             ui_settings.display.full_screen);
#endif

  if (Display::RotateSupported())
    AddEnum(_("Display orientation"), _("Rotate the display on devices that support it."),
            display_orientation_list, (unsigned)ui_settings.display.orientation);
  else
    AddDummy();

  AddEnum(_("Dark mode"), nullptr, dark_mode_list,
          (unsigned)ui_settings.dark_mode);
  SetExpertRow(DarkMode);

  AddEnum(_("InfoBox geometry"),
          _("A list of possible InfoBox layouts. Do some trials to find the best for your screen size."),
          info_box_geometry_list, (unsigned)ui_settings.info_boxes.geometry);

  AddInteger(_("InfoBox title size"), _("Zoom factor for InfoBox title and comment text"),
             _T("%d %%"), _T("%d"), 50, 150, 5,
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
               _("If true, certain InfoBoxes will have coloured text.  For example, the active waypoint "
                 "InfoBox will be blue when the glider is above final glide."),
               ui_settings.info_boxes.use_colors);
    SetExpertRow(AppInfoBoxColors);
  } else
    AddDummy();

  AddEnum(_("InfoBox border"), nullptr, infobox_border_list,
          unsigned(ui_settings.info_boxes.border_style));
  SetExpertRow(AppInfoBoxBorder);

#ifdef KOBO
  AddBoolean(_("Show Menubutton"), _("Show the Menubutton"),
             ui_settings.show_menu_button);
  SetExpertRow(ShowMenuButton);
#endif

#ifdef DRAW_MOUSE_CURSOR
  AddInteger(_("Cursor zoom"), _("Cursor zoom factor"), _T("%d x"), _T("%d x"), 1, 10, 1,
             (unsigned)ui_settings.display.cursor_size);
  AddBoolean(_("Invert cursor color"), _("Enable black cursor"),
             ui_settings.display.invert_cursor_colors);
#endif
}

bool
LayoutConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

#ifdef ANDROID
  changed |= SaveValue(FullScreen, ProfileKeys::FullScreen,
                       ui_settings.display.full_screen);
  native_view->SetFullScreen(Java::GetEnv(), ui_settings.display.full_screen);
#endif

  bool orientation_changed = false;

  if (Display::RotateSupported()) {
    orientation_changed =
      SaveValueEnum(MapOrientation, ProfileKeys::MapOrientation,
                    ui_settings.display.orientation);
    changed |= orientation_changed;
  }

  changed |= SaveValueEnum(DarkMode, ProfileKeys::DarkMode,
                           ui_settings.dark_mode);

  bool info_box_geometry_changed = false;

  info_box_geometry_changed |=
    SaveValueEnum(AppInfoBoxGeom, ProfileKeys::InfoBoxGeometry,
                  ui_settings.info_boxes.geometry);
  info_box_geometry_changed |=
    SaveValueInteger(InfoBoxTitleScale, ProfileKeys::InfoBoxTitleScale,
                  ui_settings.info_boxes.scale_title_font);

  changed |= info_box_geometry_changed;

  changed |= SaveValueEnum(AppStatusMessageAlignment, ProfileKeys::AppStatusMessageAlignment,
                           ui_settings.popup_message_position);

  changed |= SaveValueEnum(AppInfoBoxBorder, ProfileKeys::AppInfoBoxBorder,
                           ui_settings.info_boxes.border_style);

  if (HasColors())
    changed |= SaveValue(AppInfoBoxColors, ProfileKeys::AppInfoBoxColors,
                         ui_settings.info_boxes.use_colors);

#ifdef KOBO
  if (SaveValue(ShowMenuButton, ProfileKeys::ShowMenuButton,ui_settings.show_menu_button))
    require_restart = changed = true;
#endif

  DialogSettings &dialog_settings = CommonInterface::SetUISettings().dialog;
  changed |= SaveValueEnum(TabDialogStyle, ProfileKeys::AppDialogTabStyle, dialog_settings.tab_style);

#ifdef DRAW_MOUSE_CURSOR
  changed |= SaveValueInteger(CursorSize, ProfileKeys::CursorSize,
                              ui_settings.display.cursor_size);
  CommonInterface::main_window->SetCursorSize(ui_settings.display.cursor_size);

  changed |= SaveValue(CursorInverted, ProfileKeys::CursorColorsInverted, ui_settings.display.invert_cursor_colors);
  CommonInterface::main_window->SetCursorColorsInverted(ui_settings.display.invert_cursor_colors);
#endif

  if (orientation_changed) {
    assert(Display::RotateSupported());

    if (ui_settings.display.orientation == DisplayOrientation::DEFAULT)
      Display::RotateRestore();
    else {
      if (!Display::Rotate(ui_settings.display.orientation))
        LogString("Display rotation failed");
    }

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
