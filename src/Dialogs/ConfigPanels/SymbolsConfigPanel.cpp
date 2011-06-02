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

#include "SymbolsConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Appearance.hpp"
#include "Screen/Graphics.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;


void
SymbolsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpWindArrowStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Arrow head"));
    dfe->addEnumText(_("Full arrow"));
    dfe->Set(XCSoarInterface::SettingsMap().WindArrowStyle);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTrail"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("Long"));
    dfe->addEnumText(_("Short"));
    dfe->addEnumText(_("Full"));
    dfe->Set(XCSoarInterface::SettingsMap().TrailActive);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SettingsMap().EnableTrailDrift);

  wp = (WndProperty*)wf->FindByName(_T("prpSnailType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    TCHAR tmp_text[30];
    _tcscpy(tmp_text, _("Vario"));
    _tcscat(tmp_text, _T(" #1"));
    dfe->addEnumText(tmp_text);
    _tcscpy(tmp_text, _("Vario"));
    _tcscat(tmp_text, _T(" #2"));
    dfe->addEnumText(tmp_text);
    dfe->addEnumText(_("Altitude"));
    dfe->Set((int)XCSoarInterface::SettingsMap().SnailType);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpSnailWidthScale"),
                   XCSoarInterface::SettingsMap().SnailScaling);

  LoadFormProperty(*wf, _T("prpDetourCostMarker"),
                   XCSoarInterface::SettingsMap().EnableDetourCostMarker);

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayTrackBearing"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("On"));
    dfe->addEnumText(_("Auto"));
    dfe->Set(XCSoarInterface::SettingsMap().DisplayTrackBearing);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpEnableFLARMMap"),
                   XCSoarInterface::SettingsMap().EnableFLARMMap);

  wp = (WndProperty*)wf->FindByName(_T("prpAircraftSymbol"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Simple"), acSimple);
    dfe->addEnumText(_("Simple (Large)"), acSimpleLarge);
    dfe->addEnumText(_("Detailed"), acDetailed);
    dfe->Set(Appearance.AircraftSymbol);
    wp->RefreshDisplay();
  }
}


bool
SymbolsConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;

  changed |= SaveFormProperty(*wf, _T("prpWindArrowStyle"),
                              szProfileWindArrowStyle,
                              XCSoarInterface::SetSettingsMap().WindArrowStyle);

  changed |= SaveFormProperty(*wf, _T("prpTrail"),
                              szProfileSnailTrail,
                              XCSoarInterface::SetSettingsMap().TrailActive);

  changed |= SaveFormProperty(*wf, _T("prpTrailDrift"),
                              szProfileTrailDrift,
                              XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  wp = (WndProperty*)wf->FindByName(_T("prpSnailType"));
  if (wp) {
    if (XCSoarInterface::SettingsMap().SnailType != (SnailType_t)wp->GetDataField()->GetAsInteger()) {
      XCSoarInterface::SetSettingsMap().SnailType = (SnailType_t)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileSnailType, (int)XCSoarInterface::SettingsMap().SnailType);
      changed = true;
      Graphics::InitSnailTrail(XCSoarInterface::SettingsMap());
    }
  }

  bool snailscaling_changed =
      SaveFormProperty(*wf, _T("prpSnailWidthScale"),
                       szProfileSnailWidthScale,
                       XCSoarInterface::SetSettingsMap().SnailScaling);
  changed |= snailscaling_changed;
  if (snailscaling_changed)
    Graphics::InitSnailTrail(XCSoarInterface::SettingsMap());

  changed |= SaveFormProperty(*wf, _T("prpDetourCostMarker"),
                              szProfileDetourCostMarker,
                              XCSoarInterface::SetSettingsMap().EnableDetourCostMarker);

  changed |= SaveFormPropertyEnum(*wf, _T("prpDisplayTrackBearing"),
                              szProfileDisplayTrackBearing,
                              XCSoarInterface::SetSettingsMap().DisplayTrackBearing);

  changed |= SaveFormProperty(*wf, _T("prpEnableFLARMMap"),
                              szProfileEnableFLARMMap,
                              XCSoarInterface::SetSettingsMap().EnableFLARMMap);

  wp = (WndProperty*)wf->FindByName(_T("prpAircraftSymbol"));
  if (wp) {
    if (Appearance.AircraftSymbol != (AircraftSymbol_t)wp->GetDataField()->GetAsInteger()) {
      Appearance.AircraftSymbol = (AircraftSymbol_t)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileAircraftSymbol, (int)Appearance.AircraftSymbol);
      changed = true;
    }
  }

  return changed;
}
