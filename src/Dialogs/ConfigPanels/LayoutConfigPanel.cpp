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
#include "Appearance.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Screen/Graphics.hpp"
#include "Language/Language.hpp"
#include "Dialogs/XML.hpp"

static WndForm* wf = NULL;


void
LayoutConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  if (Display::RotateSupported()) {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);

    DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Portrait"));
    dfe->addEnumText(_("Landscape"));
    dfe->Set(Profile::GetDisplayOrientation());
    wp->RefreshDisplay();
  } else {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);
    wp->hide();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();

    dfe->addEnumText(_("8 Top + Bottom (Portrait)"),
                     InfoBoxLayout::ibTop4Bottom4);
    dfe->addEnumText(_("8 Bottom (Portrait)"), InfoBoxLayout::ibBottom8);
    dfe->addEnumText(_("8 Top (Portrait)"), InfoBoxLayout::ibTop8);
    dfe->addEnumText(_("8 Left + Right (Landscape)"),
                     InfoBoxLayout::ibLeft4Right4);
    dfe->addEnumText(_("8 Left (Landscape)"), InfoBoxLayout::ibLeft8);
    dfe->addEnumText(_("8 Right (Landscape)"), InfoBoxLayout::ibRight8);
    dfe->addEnumText(_("9 Right + Vario (Landscape)"), InfoBoxLayout::ibGNav);
    dfe->addEnumText(_("5 Right (Square)"), InfoBoxLayout::ibSquare);
    dfe->addEnumText(_("12 Right (Landscape)"), InfoBoxLayout::ibRight12);
    dfe->addEnumText(_("12 Bottom (Portrait)"), InfoBoxLayout::ibBottom12);
    dfe->addEnumText(_("12 Top (Portrait)"), InfoBoxLayout::ibTop12);
    dfe->addEnumText(_("24 Right (Landscape)"), InfoBoxLayout::ibRight24);
    dfe->Set(InfoBoxLayout::InfoBoxGeometry);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppStatusMessageAlignment"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Center"));
    dfe->addEnumText(_("Topleft"));
    dfe->Set(Appearance.StateMessageAlign);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Full width"));
    dfe->addEnumText(_("Scaled"));
    dfe->addEnumText(_("Scaled centered"));
    dfe->addEnumText(_("Fixed"));
    dfe->Set(DialogStyleSetting);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAppInverseInfoBox"),
                   Appearance.InverseInfoBox);

  LoadFormProperty(*wf, _T("prpAppInfoBoxColors"), Appearance.InfoBoxColors);

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Box"));
    dfe->addEnumText(_("Tab"));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTabDialogStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Text"));
    dfe->addEnumText(_("Icons"));
    dfe->Set(Appearance.DialogTabStyle);
    wp->RefreshDisplay();
  }

}


bool
LayoutConfigPanel::Save(bool &requirerestart)
{
  bool changed = false;
  WndProperty *wp;

  bool orientation_changed = false;
  if (Display::RotateSupported()) {
    wp = (WndProperty*)wf->FindByName(_T("prpDisplayOrientation"));
    assert(wp != NULL);

    const DataFieldEnum *dfe = (const DataFieldEnum *)wp->GetDataField();
    Display::orientation orientation =
      (Display::orientation)dfe->GetAsInteger();
    if (orientation != Profile::GetDisplayOrientation()) {
      Profile::SetDisplayOrientation(orientation);
      changed = true;
      orientation_changed = true;
    }
  }

  bool info_box_geometry_changed = false;
  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxGeom"));
  if (wp) {
    if (InfoBoxLayout::InfoBoxGeometry !=
        (InfoBoxLayout::Geometry)wp->GetDataField()->GetAsInteger()) {
      InfoBoxLayout::InfoBoxGeometry =
          (InfoBoxLayout::Geometry)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileInfoBoxGeometry,
                   (unsigned)InfoBoxLayout::InfoBoxGeometry);
      changed = true;
      info_box_geometry_changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppStatusMessageAlignment"));
  if (wp) {
    if (Appearance.StateMessageAlign != (StateMessageAlign_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.StateMessageAlign = (StateMessageAlign_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppStatusMessageAlignment,
                    Appearance.StateMessageAlign);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDialogStyle"));
  if (wp) {
    if (DialogStyleSetting != (DialogStyle)(wp->GetDataField()->GetAsInteger())) {
      DialogStyleSetting = (DialogStyle)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppDialogStyle, DialogStyleSetting);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppInfoBoxBorder"));
  if (wp) {
    if (Appearance.InfoBoxBorder != (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)
        (wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppInfoBoxBorder,
                    Appearance.InfoBoxBorder);
      changed = true;
      requirerestart = true;
    }
  }

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInverseInfoBox"),
                     szProfileAppInverseInfoBox, Appearance.InverseInfoBox);

  changed |= requirerestart |=
    SaveFormProperty(*wf, _T("prpAppInfoBoxColors"),
                     szProfileAppInfoBoxColors, Appearance.InfoBoxColors);

  wp = (WndProperty*)wf->FindByName(_T("prpTabDialogStyle"));
  assert(wp != NULL);
  if (Appearance.DialogTabStyle != (DialogTabStyle_t)(wp->GetDataField()->GetAsInteger())) {
    Appearance.DialogTabStyle = (DialogTabStyle_t)(wp->GetDataField()->GetAsInteger());
    Profile::Set(szProfileAppDialogTabStyle, Appearance.DialogTabStyle);
    changed = true;
  }

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

  return changed;
}
