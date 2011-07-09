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

#include "TerrainDisplayConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Form.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "Language/Language.hpp"
#include "SettingsMap.hpp"

static WndForm* wf = NULL;

static void
ShowTerrainControls(bool show)
{
  ShowFormControl(*wf, _T("prpSlopeShadingType"), show);
  ShowFormControl(*wf, _T("prpTerrainContrast"), show);
  ShowFormControl(*wf, _T("prpTerrainBrightness"), show);
  ShowFormControl(*wf, _T("prpTerrainRamp"), show);
}

void
TerrainDisplayConfigPanel::OnEnableTerrain(DataField *Sender,
                                           DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  ShowTerrainControls(df.GetAsBoolean());
}

void
TerrainDisplayConfigPanel::Init(WndForm *_wf, const SETTINGS_MAP &settings_map)
{
  const TerrainRendererSettings &terrain = settings_map.terrain;

  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  LoadFormProperty(*wf, _T("prpEnableTerrain"), terrain.enable);

  LoadFormProperty(*wf, _T("prpEnableTopography"),
                   settings_map.EnableTopography);

  wp = (WndProperty*)wf->FindByName(_T("prpSlopeShadingType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Off"));
    dfe->addEnumText(_("Fixed"));
    dfe->addEnumText(_("Sun"));
    dfe->addEnumText(_("Wind"));
    dfe->Set(terrain.slope_shading);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpTerrainContrast"),
                   terrain.contrast * 100 / 255);

  LoadFormProperty(*wf, _T("prpTerrainBrightness"),
                   terrain.brightness * 100 / 255);

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainRamp"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Low lands"));
    dfe->addEnumText(_("Mountainous"));
    dfe->addEnumText(_("Imhof 7"));
    dfe->addEnumText(_("Imhof 4"));
    dfe->addEnumText(_("Imhof 12"));
    dfe->addEnumText(_("Imhof Atlas"));
    dfe->addEnumText(_("ICAO"));
    dfe->addEnumText(_("Grey"));
    dfe->Set(terrain.ramp);
    wp->RefreshDisplay();
  }

  ShowTerrainControls(terrain.enable);
}


bool
TerrainDisplayConfigPanel::Save(SETTINGS_MAP &settings_map)
{
  TerrainRendererSettings &terrain = settings_map.terrain;

  bool changed = false;
  WndProperty *wp;

  changed |= SaveFormProperty(*wf, _T("prpEnableTerrain"),
                              szProfileDrawTerrain,
                              terrain.enable);

  changed |= SaveFormProperty(*wf, _T("prpEnableTopography"),
                              szProfileDrawTopography,
                              settings_map.EnableTopography);

  wp = (WndProperty*)wf->FindByName(_T("prpSlopeShadingType"));
  if (wp) {
    if (terrain.slope_shading != wp->GetDataField()->GetAsInteger()) {
      terrain.slope_shading =
        (SlopeShadingType_t)wp->GetDataField()->GetAsInteger();
      Profile::Set(szProfileSlopeShadingType, terrain.slope_shading);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainContrast"));
  if (wp) {
    if (terrain.contrast * 100 / 255 != wp->GetDataField()->GetAsInteger()) {
      terrain.contrast = (short)(wp->GetDataField()->GetAsInteger() * 255 / 100);
      Profile::Set(szProfileTerrainContrast, terrain.contrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTerrainBrightness"));
  if (wp) {
    if (terrain.brightness * 100 / 255 != wp->GetDataField()->GetAsInteger()) {
      terrain.brightness = (short)(wp->GetDataField()->GetAsInteger() * 255 / 100);
      Profile::Set(szProfileTerrainBrightness, terrain.brightness);
      changed = true;
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpTerrainRamp"), szProfileTerrainRamp,
                              terrain.ramp);

  return changed;
}
