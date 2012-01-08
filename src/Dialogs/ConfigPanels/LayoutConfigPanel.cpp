/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "LayoutConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DisplayConfig.hpp"
#include "DataField/Enum.hpp"
#include "Hardware/Display.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Screen/Graphics.hpp"
#include "Language/Language.hpp"
#include "Dialogs/XML.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  DisplayOrientation,
  AppInfoBoxGeom,
  AppFlarmLocation,
  TabDialogStyle,
  AppStatusMessageAlignment,
  DialogStyle,
  AppInverseInfoBox,
  AppInfoBoxColors,
  AppInfoBoxBorder
};

const TCHAR *display_orientation_help = N_("Rotate the display on devices that support it.");
static const StaticEnumChoice display_orientation_list[] = {
  { Display::ORIENTATION_DEFAULT, N_("Default"), display_orientation_help },
  { Display::ORIENTATION_PORTRAIT, N_("Portrait"), display_orientation_help },
  { Display::ORIENTATION_LANDSCAPE, N_("Landscape"), display_orientation_help },
  { Display::ORIENTATION_REVERSE_PORTRAIT, N_("Reverse Portrait"), display_orientation_help },
  { Display::ORIENTATION_REVERSE_LANDSCAPE, N_("Reverse Landscape"), display_orientation_help },
  { 0 }
};

static const StaticEnumChoice info_box_geometry_list[] = {
  { InfoBoxLayout::ibTop4Bottom4, N_("8 Top + Bottom (Portrait)") },
  { InfoBoxLayout::ibBottom8, N_("8 Bottom (Portrait)") },
  { InfoBoxLayout::ibBottom8Vario, N_("8 Bottom + Vario (Portrait)") },
  { InfoBoxLayout::ibTop8, N_("8 Top (Portrait)") },
  { InfoBoxLayout::ibLeft4Right4, N_("8 Left + Right (Landscape)") },
  { InfoBoxLayout::ibLeft8, N_("8 Left (Landscape)") },
  { InfoBoxLayout::ibRight8, N_("8 Right (Landscape)") },
  { InfoBoxLayout::ibGNav, N_("9 Right + Vario (Landscape)") },
  { InfoBoxLayout::ibGNav2, N_("9 Left + Right (Landscape)") },
  { InfoBoxLayout::ibSquare, N_("5 Right (Square)") },
  { InfoBoxLayout::ibRight12, N_("12 Right (Landscape)") },
  { InfoBoxLayout::ibBottom12, N_("12 Bottom (Portrait)") },
  { InfoBoxLayout::ibTop12, N_("12 Top (Portrait)") },
  { InfoBoxLayout::ibRight24, N_("24 Right (Landscape)") },
  { 0 }
};

const TCHAR *flarm_display_help = N_("Choose a location for the FLARM display.");
static const StaticEnumChoice flarm_display_location_list[] = {
  { UISettings::flAuto, N_("Auto (follow infoboxes)"), flarm_display_help },
  { UISettings::flTopLeft, N_("Top Left"), flarm_display_help },
  { UISettings::flTopRight, N_("Top Right"), flarm_display_help },
  { UISettings::flBottomLeft, N_("Bottom Left"), flarm_display_help },
  { UISettings::flBottomRight, N_("Bottom Right"), flarm_display_help },
  { UISettings::flCentreTop, N_("Centre Top"), flarm_display_help },
  { UISettings::flCentreBottom, N_("Centre Bottom"), flarm_display_help },
  { 0 }
};

static const StaticEnumChoice tabdialog_style_list[] = {
  { 0, N_("Text"), N_("Show text on tabbed dialogs.") },
  { 1, N_("Icons"), N_("Show icons on tabbed dialogs.")},
  { 0 }
};

static const StaticEnumChoice popup_msg_position_list[] = {
  { 0, N_("Center"), N_("Center the status message boxes.") },
  { 1, N_("Topleft"), N_("Show status message boxes in the top left corner.") },
  { 0 }
};

const TCHAR *dialog_style_help = N_("Choose the display size of dialogs.");
static const StaticEnumChoice dialog_style_list[] = {
  { 0, N_("Full width"), dialog_style_help },
  { 1, N_("Scaled"), dialog_style_help },
  { 2, N_("Scaled centered"), dialog_style_help },
  { 3, N_("Fixed"), dialog_style_help },
  { 0 }
};

static const StaticEnumChoice infobox_border_list[] = {
  { 0, N_("Box"), N_("Draws boxes around each InfoBox.") },
  { 1, N_("Tab"), N_("Draws a tab at the top of the InfoBox across the title.") },
  { 0 }
};

class LayoutConfigPanel : public RowFormWidget {
public:
  LayoutConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(110)) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
LayoutConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  WndProperty *wp = AddEnum(_("Display orientation"), _T(""),
                            display_orientation_list, Profile::GetDisplayOrientation());
  wp->set_visible(Display::RotateSupported());

  AddEnum(_("InfoBox geometry"),
          _("A list of possible InfoBox layouts. Do some trials to find the best for your screen size."),
          info_box_geometry_list, InfoBoxLayout::InfoBoxGeometry);

  // Expert item (TODO)
  AddEnum(_("FLARM display"), _T(""),
          flarm_display_location_list, ui_settings.flarm_location);

  AddEnum(_("Tab dialog style"), _T(""),
          tabdialog_style_list, CommonInterface::GetUISettings().dialog.tab_style);

  // Expert item
  AddEnum(_("Message display"), _T(""),
          popup_msg_position_list, ui_settings.popup_message_position);
          // Expert item
  AddEnum(_("Dialog size"), _T(""),
          dialog_style_list, dialog_style_setting);

  // Expert item
  AddBoolean(_("Inverse InfoBoxes"), _("If true, the InfoBoxes are white on black, otherwise black on white."),
             ui_settings.info_boxes.inverse);

  // Expert item
  AddBoolean(_("Colored InfoBoxes"),
             _("If true, certain InfoBoxes will have coloured text.  For example, the active waypoint "
                 "InfoBox will be blue when the glider is above final glide."),
             ui_settings.info_boxes.use_colors);

  // Expert item
  AddEnum(_("InfoBox border"), _T(""), infobox_border_list, ui_settings.info_boxes.border_style);
}

bool
LayoutConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  bool orientation_changed = false;

  if (Display::RotateSupported()) {
    Display::orientation orientation = Profile::GetDisplayOrientation();
    if (SaveValueEnum(DisplayOrientation, orientation)) {
      Profile::SetDisplayOrientation(orientation);
      changed = true;
      orientation_changed = true;
    }
  }

  bool info_box_geometry_changed = false;

  info_box_geometry_changed |=
    SaveValueEnum(AppInfoBoxGeom, szProfileInfoBoxGeometry, InfoBoxLayout::InfoBoxGeometry);

  info_box_geometry_changed |=
    SaveValueEnum(AppFlarmLocation, szProfileFlarmLocation, ui_settings.flarm_location);

  changed |= info_box_geometry_changed;

  changed |= SaveValueEnum(AppStatusMessageAlignment, szProfileAppStatusMessageAlignment,
                           ui_settings.popup_message_position);

  changed |= SaveValueEnum(DialogStyle, szProfileAppDialogStyle, dialog_style_setting);

  changed |= require_restart |=
    SaveValueEnum(AppInfoBoxBorder, szProfileAppInfoBoxBorder, ui_settings.info_boxes.border_style);

  changed |= require_restart |=
    SaveValue(AppInverseInfoBox, szProfileAppInverseInfoBox, ui_settings.info_boxes.inverse);

  changed |= require_restart |=
    SaveValue(AppInfoBoxColors, szProfileAppInfoBoxColors, ui_settings.info_boxes.use_colors);

  DialogSettings &dialog_settings = CommonInterface::SetUISettings().dialog;
  changed |= SaveValueEnum(TabDialogStyle, szProfileAppDialogTabStyle, dialog_settings.tab_style);

  if (orientation_changed) {
    assert(Display::RotateSupported());

    Display::orientation orientation = Profile::GetDisplayOrientation();
    if (orientation == Display::ORIENTATION_DEFAULT)
      Display::RotateRestore();
    else {
      if (!Display::Rotate(orientation))
        LogStartUp(_T("Display rotation failed"));
    }
  } else if (info_box_geometry_changed)
    XCSoarInterface::main_window.ReinitialiseLayout();

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateLayoutConfigPanel()
{
  return new LayoutConfigPanel();
}
