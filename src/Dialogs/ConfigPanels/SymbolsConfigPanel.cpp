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
#include "Screen/Graphics.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;

static void
ShowTrailControls(bool show)
{
  ShowFormControl(*wf, _T("prpTrailDrift"), show);
  ShowFormControl(*wf, _T("prpSnailType"), show);
  ShowFormControl(*wf, _T("prpSnailWidthScale"), show);
}

void
SymbolsConfigPanel::OnTrailLength(DataField *Sender,
                                  DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  TrailLength trail_length = (TrailLength)df.GetAsInteger();
  ShowTrailControls(trail_length != TRAIL_OFF);
}

void
SymbolsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const SETTINGS_MAP &settings_map = CommonInterface::SettingsMap();

  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpWindArrowStyle"));
  assert(wp != NULL);
  DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Arrow head"));
  dfe->addEnumText(_("Full arrow"));
  dfe->Set(settings_map.wind_arrow_style);
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpTrail"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Off"), TRAIL_OFF);
  dfe->addEnumText(_("Long"), TRAIL_LONG);
  dfe->addEnumText(_("Short"), TRAIL_SHORT);
  dfe->addEnumText(_("Full"), TRAIL_FULL);
  dfe->Set(settings_map.trail_length);
  wp->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpTrailDrift"),
                   settings_map.trail_drift_enabled);

  wp = (WndProperty*)wf->FindByName(_T("prpSnailType"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  TCHAR tmp_text[30];
  _tcscpy(tmp_text, _("Vario"));
  _tcscat(tmp_text, _T(" #1"));
  dfe->addEnumText(tmp_text);
  _tcscpy(tmp_text, _("Vario"));
  _tcscat(tmp_text, _T(" #2"));
  dfe->addEnumText(tmp_text);
  dfe->addEnumText(_("Altitude"));
  dfe->Set((int)settings_map.snail_type);
  wp->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpSnailWidthScale"),
                   settings_map.snail_scaling_enabled);

  LoadFormProperty(*wf, _T("prpDetourCostMarker"),
                   settings_map.detour_cost_markers_enabled);

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayTrackBearing"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Off"));
  dfe->addEnumText(_("On"));
  dfe->addEnumText(_("Auto"));
  dfe->Set(settings_map.display_track_bearing);
  wp->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpEnableFLARMMap"),
                   settings_map.show_flarm_on_map);

  wp = (WndProperty*)wf->FindByName(_T("prpAircraftSymbol"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Simple"), acSimple);
  dfe->addEnumText(_("Simple (large)"), acSimpleLarge);
  dfe->addEnumText(_("Detailed"), acDetailed);
  dfe->Set(settings_map.aircraft_symbol);
  wp->RefreshDisplay();

  ShowTrailControls(settings_map.trail_length != TRAIL_OFF);
}


bool
SymbolsConfigPanel::Save()
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();
  bool changed = false;
  WndProperty *wp;

  changed |= SaveFormProperty(*wf, _T("prpWindArrowStyle"),
                              szProfileWindArrowStyle,
                              settings_map.wind_arrow_style);

  changed |= SaveFormPropertyEnum(*wf, _T("prpTrail"),
                                  szProfileSnailTrail,
                                  settings_map.trail_length);

  changed |= SaveFormProperty(*wf, _T("prpTrailDrift"),
                              szProfileTrailDrift,
                              settings_map.trail_drift_enabled);

  wp = (WndProperty*)wf->FindByName(_T("prpSnailType"));
  assert(wp != NULL);
  if (settings_map.snail_type != (SnailType)wp->GetDataField()->GetAsInteger()) {
    settings_map.snail_type = (SnailType)wp->GetDataField()->GetAsInteger();
    Profile::Set(szProfileSnailType, (int)settings_map.snail_type);
    changed = true;
    Graphics::InitSnailTrail(settings_map);
  }

  bool snailscaling_changed =
      SaveFormProperty(*wf, _T("prpSnailWidthScale"),
                       szProfileSnailWidthScale,
                       settings_map.snail_scaling_enabled);
  changed |= snailscaling_changed;
  if (snailscaling_changed)
    Graphics::InitSnailTrail(settings_map);

  changed |= SaveFormProperty(*wf, _T("prpDetourCostMarker"),
                              szProfileDetourCostMarker,
                              settings_map.detour_cost_markers_enabled);

  changed |= SaveFormPropertyEnum(*wf, _T("prpDisplayTrackBearing"),
                              szProfileDisplayTrackBearing,
                              settings_map.display_track_bearing);

  changed |= SaveFormProperty(*wf, _T("prpEnableFLARMMap"),
                              szProfileEnableFLARMMap,
                              settings_map.show_flarm_on_map);

  wp = (WndProperty*)wf->FindByName(_T("prpAircraftSymbol"));
  assert(wp != NULL);
  if (settings_map.aircraft_symbol != (AircraftSymbol)wp->GetDataField()->GetAsInteger()) {
    settings_map.aircraft_symbol = (AircraftSymbol)wp->GetDataField()->GetAsInteger();
    Profile::Set(szProfileAircraftSymbol, (int)settings_map.aircraft_symbol);
    changed = true;
  }

  return changed;
}
