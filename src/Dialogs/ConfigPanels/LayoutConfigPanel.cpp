/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Hardware/Display.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Screen/Graphics.hpp"
#include "Language/Language.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

static const StaticEnumChoice display_orientation_list[] = {
  { Display::ORIENTATION_DEFAULT, N_("Default") },
  { Display::ORIENTATION_PORTRAIT, N_("Portrait") },
  { Display::ORIENTATION_LANDSCAPE, N_("Landscape") },
  { Display::ORIENTATION_REVERSE_PORTRAIT, N_("Reverse Portrait") },
  { Display::ORIENTATION_REVERSE_LANDSCAPE, N_("Reverse Landscape") },
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

class LayoutConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
LayoutConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
LayoutConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
LayoutConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_LAYOUTCONFIGPANEL") :
                               _T("IDR_XML_LAYOUTCONFIGPANEL_L"));

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  if (Display::RotateSupported())
    LoadFormProperty(form, _T("prpDisplayOrientation"),
                     display_orientation_list,
                     Profile::GetDisplayOrientation());
  else
    ShowFormControl(form, _T("prpDisplayOrientation"), false);

  LoadFormProperty(form, _T("prpAppInfoBoxGeom"),
                   info_box_geometry_list, InfoBoxLayout::InfoBoxGeometry);

  WndProperty *wp = (WndProperty*)form.FindByName(_T("prpAppFlarmLocation"));
  assert(wp != NULL);
  DataFieldEnum *dfe = (DataFieldEnum*)wp->GetDataField();
  assert(dfe != NULL);
  dfe->addEnumText(_("Auto (follow infoboxes)"), UISettings::flAuto);
  dfe->addEnumText(_("Top Left"), UISettings::flTopLeft);
  dfe->addEnumText(_("Top Right"), UISettings::flTopRight);
  dfe->addEnumText(_("Bottom Left"), UISettings::flBottomLeft);
  dfe->addEnumText(_("Bottom Right"), UISettings::flBottomRight);
  dfe->addEnumText(_("Centre Top"), UISettings::flCentreTop);
  dfe->addEnumText(_("Centre Bottom"), UISettings::flCentreBottom);
  dfe->Set(ui_settings.flarm_location);
  wp->RefreshDisplay();

  wp = (WndProperty*)form.FindByName(_T("prpAppStatusMessageAlignment"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  assert(dfe != NULL);
  dfe->addEnumText(_("Center"));
  dfe->addEnumText(_("Topleft"));
  dfe->Set(ui_settings.popup_message_position);
  wp->RefreshDisplay();

  wp = (WndProperty*)form.FindByName(_T("prpDialogStyle"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  assert(dfe != NULL);
  dfe->addEnumText(_("Full width"));
  dfe->addEnumText(_("Scaled"));
  dfe->addEnumText(_("Scaled centered"));
  dfe->addEnumText(_("Fixed"));
  dfe->Set(dialog_style_setting);
  wp->RefreshDisplay();

  LoadFormProperty(form, _T("prpAppInverseInfoBox"),
                   ui_settings.info_boxes.inverse);

  LoadFormProperty(form, _T("prpAppInfoBoxColors"),
                   ui_settings.info_boxes.use_colors);

  wp = (WndProperty*)form.FindByName(_T("prpAppInfoBoxBorder"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  assert(dfe != NULL);
  dfe->addEnumText(_("Box"));
  dfe->addEnumText(_("Tab"));
  dfe->Set(ui_settings.info_boxes.border_style);
  wp->RefreshDisplay();

  wp = (WndProperty*)form.FindByName(_T("prpTabDialogStyle"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  assert(dfe != NULL);
  dfe->addEnumText(_("Text"));
  dfe->addEnumText(_("Icons"));
  dfe->Set(CommonInterface::GetUISettings().dialog.tab_style);
  wp->RefreshDisplay();
}

bool
LayoutConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  WndProperty *wp;

  bool orientation_changed = false;

  if (Display::RotateSupported()) {
    wp = (WndProperty*)form.FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);

    const DataFieldEnum *dfe = (const DataFieldEnum *)wp->GetDataField();
    assert(dfe != NULL);
    Display::orientation orientation =
      (Display::orientation)dfe->GetAsInteger();
    if (orientation != Profile::GetDisplayOrientation()) {
      Profile::SetDisplayOrientation(orientation);
      changed = true;
      orientation_changed = true;
    }
  }

  bool info_box_geometry_changed = false;

  info_box_geometry_changed |=
    SaveFormPropertyEnum(form, _T("prpAppInfoBoxGeom"),
                         szProfileInfoBoxGeometry,
                         InfoBoxLayout::InfoBoxGeometry);

  info_box_geometry_changed |=
    SaveFormPropertyEnum(form, _T("prpAppFlarmLocation"),
                         szProfileFlarmLocation, ui_settings.flarm_location);

  changed |= info_box_geometry_changed;

  changed |= SaveFormPropertyEnum(form, _T("prpAppStatusMessageAlignment"),
                                  szProfileAppStatusMessageAlignment,
                                  ui_settings.popup_message_position);

  changed |= SaveFormPropertyEnum(form, _T("prpDialogStyle"),
                                  szProfileAppDialogStyle,
                                  dialog_style_setting);

  changed |= require_restart |=
    SaveFormPropertyEnum(form, _T("prpAppInfoBoxBorder"),
                         szProfileAppInfoBoxBorder,
                         ui_settings.info_boxes.border_style);

  changed |= require_restart |=
    SaveFormProperty(form, _T("prpAppInverseInfoBox"),
                     szProfileAppInverseInfoBox,
                     ui_settings.info_boxes.inverse);

  changed |= require_restart |=
    SaveFormProperty(form, _T("prpAppInfoBoxColors"),
                     szProfileAppInfoBoxColors,
                     ui_settings.info_boxes.use_colors);

  DialogSettings &dialog_settings = CommonInterface::SetUISettings().dialog;
  changed |= SaveFormPropertyEnum(form, _T("prpTabDialogStyle"),
                                  szProfileAppDialogTabStyle,
                                  dialog_settings.tab_style);

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
