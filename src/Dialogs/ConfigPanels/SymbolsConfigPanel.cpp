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
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "DataField/Base.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"

class SymbolsConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void ShowTrailControls(bool show);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static SymbolsConfigPanel *instance;

void
SymbolsConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
SymbolsConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
SymbolsConfigPanel::ShowTrailControls(bool show)
{
  ShowFormControl(form, _T("prpTrailDrift"), show);
  ShowFormControl(form, _T("prpSnailType"), show);
  ShowFormControl(form, _T("prpSnailWidthScale"), show);
}

static void
OnTrailLength(DataField *Sender,
              DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  TrailLength trail_length = (TrailLength)df.GetAsInteger();
  instance->ShowTrailControls(trail_length != TRAIL_OFF);
}

gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnTrailLength),
  DeclareCallBackEntry(NULL)
};

void
SymbolsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_SYMBOLSCONFIGPANEL") :
                               _T("IDR_XML_SYMBOLSCONFIGPANEL_L"));

  const MapSettings &settings_map = CommonInterface::GetMapSettings();

  WndProperty *wp;

  wp = (WndProperty*)form.FindByName(_T("prpWindArrowStyle"));
  assert(wp != NULL);
  DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Arrow head"));
  dfe->addEnumText(_("Full arrow"));
  dfe->Set(settings_map.wind_arrow_style);
  wp->RefreshDisplay();

  wp = (WndProperty*)form.FindByName(_T("prpTrail"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Off"), TRAIL_OFF);
  dfe->addEnumText(_("Long"), TRAIL_LONG);
  dfe->addEnumText(_("Short"), TRAIL_SHORT);
  dfe->addEnumText(_("Full"), TRAIL_FULL);
  dfe->Set(settings_map.trail_length);
  wp->RefreshDisplay();

  LoadFormProperty(form, _T("prpTrailDrift"),
                   settings_map.trail_drift_enabled);

  wp = (WndProperty*)form.FindByName(_T("prpSnailType"));
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

  LoadFormProperty(form, _T("prpSnailWidthScale"),
                   settings_map.snail_scaling_enabled);

  LoadFormProperty(form, _T("prpDetourCostMarker"),
                   settings_map.detour_cost_markers_enabled);

  wp = (WndProperty*)form.FindByName(_T("prpDisplayTrackBearing"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->addEnumText(_("Off"));
  dfe->addEnumText(_("On"));
  dfe->addEnumText(_("Auto"));
  dfe->Set(settings_map.display_track_bearing);
  wp->RefreshDisplay();

  LoadFormProperty(form, _T("prpEnableFLARMMap"),
                   settings_map.show_flarm_on_map);

  wp = (WndProperty*)form.FindByName(_T("prpAircraftSymbol"));
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
SymbolsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  MapSettings &settings_map = CommonInterface::SetMapSettings();

  changed |= SaveFormProperty(form, _T("prpWindArrowStyle"),
                              szProfileWindArrowStyle,
                              settings_map.wind_arrow_style);

  changed |= SaveFormPropertyEnum(form, _T("prpTrail"),
                                  szProfileSnailTrail,
                                  settings_map.trail_length);

  changed |= SaveFormProperty(form, _T("prpTrailDrift"),
                              szProfileTrailDrift,
                              settings_map.trail_drift_enabled);

  changed |= SaveFormPropertyEnum(form, _T("prpSnailType"),
                                  szProfileSnailType,
                                  settings_map.snail_type);

  changed |= SaveFormProperty(form, _T("prpSnailWidthScale"),
                              szProfileSnailWidthScale,
                              settings_map.snail_scaling_enabled);

  changed |= SaveFormProperty(form, _T("prpDetourCostMarker"),
                              szProfileDetourCostMarker,
                              settings_map.detour_cost_markers_enabled);

  changed |= SaveFormPropertyEnum(form, _T("prpDisplayTrackBearing"),
                              szProfileDisplayTrackBearing,
                              settings_map.display_track_bearing);

  changed |= SaveFormProperty(form, _T("prpEnableFLARMMap"),
                              szProfileEnableFLARMMap,
                              settings_map.show_flarm_on_map);

  changed |= SaveFormPropertyEnum(form, _T("prpAircraftSymbol"),
                                  szProfileAircraftSymbol,
                                  settings_map.aircraft_symbol);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateSymbolsConfigPanel()
{
  return new SymbolsConfigPanel();
}
