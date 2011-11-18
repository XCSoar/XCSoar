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
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"

static WndForm* wf = NULL;
static TerrainRendererSettings terrain_settings;

static void
ShowTerrainControls()
{
  bool show = terrain_settings.enable;
  ShowFormControl(*wf, _T("prpSlopeShadingType"), show);
  ShowFormControl(*wf, _T("prpTerrainContrast"), show);
  ShowFormControl(*wf, _T("prpTerrainBrightness"), show);
  ShowFormControl(*wf, _T("prpTerrainRamp"), show);
  ShowFormControl(*wf, _T("frmPreview"), show);
}

static short
ByteToPercent(short byte)
{
  return (byte * 200 + 100) / 510;
}

static short
PercentToByte(short percent)
{
  return (percent * 510 + 255) / 200;
}

void
TerrainDisplayConfigPanel::OnEnableTerrain(DataField *Sender,
                                           DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  terrain_settings.enable = df.GetAsBoolean();
  ShowTerrainControls();
}

void
TerrainDisplayConfigPanel::OnChangeTerrain(gcc_unused DataField *Sender,
                                           gcc_unused DataField::DataAccessKind_t Mode)
{
  GetFormValueEnum(*wf, _T("prpSlopeShadingType"),
                   terrain_settings.slope_shading);
  terrain_settings.contrast =
    PercentToByte(GetFormValueInteger(*wf, _T("prpTerrainContrast")));
  terrain_settings.brightness =
    PercentToByte(GetFormValueInteger(*wf, _T("prpTerrainBrightness")));
  terrain_settings.ramp =
    (short) GetFormValueInteger(*wf, _T("prpTerrainRamp"));

  // invalidate terrain preview
  PaintWindow *w = (PaintWindow *)wf->FindByName(_T("frmPreview"));
  if (w)
    w->invalidate();
}

void
TerrainDisplayConfigPanel::OnPreviewPaint(gcc_unused WndOwnerDrawFrame *Sender,
                                          Canvas &canvas)
{
  TerrainRenderer renderer(terrain);
  renderer.SetSettings(terrain_settings);

  MapWindowProjection projection =
    XCSoarInterface::main_window.map->VisibleProjection();
  projection.SetScreenSize(canvas.get_width(), canvas.get_height());
  projection.SetScreenOrigin(canvas.get_width() / 2, canvas.get_height() / 2);

  Angle sun_azimuth(Angle::Degrees(fixed(-45)));
  if (terrain_settings.slope_shading == sstSun)
    sun_azimuth = XCSoarInterface::Calculated().sun_azimuth;

  renderer.Generate(projection, sun_azimuth);
  renderer.Draw(canvas, projection);
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
                   settings_map.topography_enabled);

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
                   ByteToPercent(terrain.contrast));

  LoadFormProperty(*wf, _T("prpTerrainBrightness"),
                   ByteToPercent(terrain.brightness));

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

  terrain_settings = terrain;
  ShowTerrainControls();
}

bool
TerrainDisplayConfigPanel::Save(SETTINGS_MAP &settings_map)
{
  bool changed = (settings_map.terrain != terrain_settings);

  settings_map.terrain = terrain_settings;
  Profile::Set(szProfileDrawTerrain, terrain_settings.enable);
  Profile::Set(szProfileTerrainContrast, terrain_settings.contrast);
  Profile::Set(szProfileTerrainBrightness, terrain_settings.brightness);
  Profile::Set(szProfileTerrainRamp, terrain_settings.ramp);
  Profile::Set(szProfileSlopeShadingType, terrain_settings.slope_shading);

  changed |= SaveFormProperty(*wf, _T("prpEnableTopography"),
                              szProfileDrawTopography,
                              settings_map.topography_enabled);

  return changed;
}
