/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

enum ControlIndex {
  EnableTerrain,
  EnableTopography,
  TerrainColors,
  TerrainSlopeShading,
  TerrainContrast,
  TerrainBrightness,
  TerrainContours,
  TerrainPreview,
};

class TerrainPreviewWindow : public PaintWindow {
  TerrainRenderer renderer;

public:
  TerrainPreviewWindow(const RasterTerrain &terrain)
    :renderer(terrain) {}

  void SetSettings(const TerrainRendererSettings &settings) {
    renderer.SetSettings(settings);
    renderer.Flush();
    Invalidate();
  }

  virtual void OnPaint(Canvas &canvas) override;
};

class TerrainDisplayConfigPanel final
  : public RowFormWidget, DataFieldListener {

protected:
  TerrainRendererSettings terrain_settings;

public:
  TerrainDisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void ShowTerrainControls();
  void OnPreviewPaint(Canvas &canvas);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

protected:
  void UpdateTerrainPreview();

  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TerrainDisplayConfigPanel *instance;

void
TerrainDisplayConfigPanel::ShowTerrainControls()
{
  bool show = terrain_settings.enable;
  SetRowVisible(TerrainColors, show);
  SetRowVisible(TerrainSlopeShading, show);
  SetRowVisible(TerrainContrast, show);
  SetRowVisible(TerrainBrightness, show);
  SetRowVisible(TerrainContours, show);
  if (terrain != nullptr)
    SetRowVisible(TerrainPreview, show);
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
TerrainDisplayConfigPanel::UpdateTerrainPreview()
{
  terrain_settings.slope_shading = (SlopeShading)
    GetValueInteger(TerrainSlopeShading);
  terrain_settings.contrast = PercentToByte(GetValueInteger(TerrainContrast));
  terrain_settings.brightness =
    PercentToByte(GetValueInteger(TerrainBrightness));
  terrain_settings.ramp = GetValueInteger(TerrainColors);
  terrain_settings.contours = (Contours)
    GetValueInteger(TerrainContours);

  // Invalidate terrain preview
  if (terrain != nullptr)
    ((TerrainPreviewWindow &)GetRow(TerrainPreview)).SetSettings(terrain_settings);
}

void
TerrainDisplayConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(EnableTerrain, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    terrain_settings.enable = dfb.GetAsBoolean();
    ShowTerrainControls();
  } else {
    UpdateTerrainPreview();
  }
}

void
TerrainPreviewWindow::OnPaint(Canvas &canvas)
{
  const GlueMapWindow *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  MapWindowProjection projection = map->VisibleProjection();
  if (!projection.IsValid()) {
    /* TODO: initialise projection to middle of map instead of bailing
       out */
    canvas.ClearWhite();
    return;
  }

  projection.SetScreenSize(canvas.GetSize());
  projection.SetScreenOrigin(canvas.GetWidth() / 2, canvas.GetHeight() / 2);

  Angle sun_azimuth(Angle::Degrees(-45));
  if (renderer.GetSettings().slope_shading == SlopeShading::SUN &&
      CommonInterface::Calculated().sun_data_available)
    sun_azimuth = CommonInterface::Calculated().sun_azimuth;

  renderer.Generate(projection, sun_azimuth);

#ifdef ENABLE_OPENGL
  /* enable clipping because the OpenGL terrain renderer uses a large
     texture that exceeds the window dimensions */
  GLCanvasScissor scissor(canvas);
#endif

  renderer.Draw(canvas, projection);
}

void
TerrainDisplayConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  RowFormWidget::Prepare(parent, rc);

  const MapSettings &settings_map = CommonInterface::GetMapSettings();
  const TerrainRendererSettings &terrain = settings_map.terrain;

  AddBoolean(_("Terrain display"),
             _("Draw a digital elevation terrain on the map."),
             terrain.enable);
  GetDataField(EnableTerrain).SetListener(this);

  AddBoolean(_("Topography display"),
             _("Draw topographical features (roads, rivers, lakes etc.) on the map."),
             settings_map.topography_enabled);

  static constexpr StaticEnumChoice terrain_ramp_list[] = {
    { 0, N_("Low lands"), },
    { 1, N_("Mountainous"), },
    { 2, N_("Imhof 7"), },
    { 3, N_("Imhof 4"), },
    { 4, N_("Imhof 12"), },
    { 5, N_("Imhof Atlas"), },
    { 6, N_("ICAO"), },
    { 9, N_("Vibrant"), },
    { 7, N_("Grey"), },
    { 8, N_("White"), },
    {10, N_("Sandstone"), },
    {11, N_("Pastel"), },
    {12, N_("Italian Avioportolano VFR Chart"), },
    {13, N_("German DFS VFR Chart"), },
    {14, N_("French SIA VFR Chart"), },
    { 0 }
  };

  AddEnum(_("Terrain colors"),
          _("Defines the color ramp used in terrain rendering."),
          terrain_ramp_list, terrain.ramp);
  GetDataField(TerrainColors).SetListener(this);

  static constexpr StaticEnumChoice slope_shading_list[] = {
    { (unsigned)SlopeShading::OFF, N_("Off"), },
    { (unsigned)SlopeShading::FIXED, N_("Fixed"), },
    { (unsigned)SlopeShading::SUN, N_("Sun"), },
    { (unsigned)SlopeShading::WIND, N_("Wind"), },
    { 0 }
  };

  AddEnum(_("Slope shading"),
          _("The terrain can be shaded among slopes to indicate either wind direction, sun position or a fixed shading from North-West."),
          slope_shading_list, (unsigned)terrain.slope_shading);
  GetDataField(TerrainSlopeShading).SetListener(this);
  SetExpertRow(TerrainSlopeShading);

  AddInteger(_("Terrain contrast"),
             _("Defines the amount of Phong shading in the terrain rendering.  Use large values to emphasise terrain slope, smaller values if flying in steep mountains."),
             _T("%d %%"), _T("%d %%"), 0, 100, 5,
             ByteToPercent(terrain.contrast));
  GetDataField(TerrainContrast).SetListener(this);
  SetExpertRow(TerrainContrast);

  AddInteger(_("Terrain brightness"),
             _("Defines the brightness (whiteness) of the terrain rendering.  This controls the average illumination of the terrain."),
             _T("%d %%"), _T("%d %%"), 0, 100, 5,
             ByteToPercent(terrain.brightness));
  GetDataField(TerrainBrightness).SetListener(this);
  SetExpertRow(TerrainBrightness);

  // JMW using enum here instead of bool so can provide more contour rendering
  // options later
  static constexpr StaticEnumChoice contours_list[] = {
    { (unsigned)Contours::OFF, N_("Off"), },
    { (unsigned)Contours::ON, N_("On"), },
    { 0 }
  };

  AddEnum(_("Contours"),
          _("If enabled, draws contour lines on the terrain."),
          contours_list, (unsigned)terrain.contours);
  GetDataField(TerrainContours).SetListener(this);
  SetExpertRow(TerrainContours);

  if (::terrain != nullptr) {
    WindowStyle style;
    style.Border();

    TerrainPreviewWindow *preview = new TerrainPreviewWindow(*::terrain);
    preview->Create((ContainerWindow &)GetWindow(), {0, 0, 100, 100}, style);
    AddRemaining(preview);
  }

  terrain_settings = terrain;
  ShowTerrainControls();
  UpdateTerrainPreview();
}

bool
TerrainDisplayConfigPanel::Save(bool &_changed)
{
  MapSettings &settings_map = CommonInterface::SetMapSettings();

  bool changed = false;
  changed = (settings_map.terrain != terrain_settings);

  settings_map.terrain = terrain_settings;
  Profile::Set(ProfileKeys::DrawTerrain, terrain_settings.enable);
  Profile::Set(ProfileKeys::TerrainContrast, terrain_settings.contrast);
  Profile::Set(ProfileKeys::TerrainBrightness, terrain_settings.brightness);
  Profile::Set(ProfileKeys::TerrainRamp, terrain_settings.ramp);
  Profile::SetEnum(ProfileKeys::SlopeShadingType, terrain_settings.slope_shading);
  Profile::SetEnum(ProfileKeys::TerrainContours, terrain_settings.contours);

  changed |= SaveValue(EnableTopography, ProfileKeys::DrawTopography,
                       settings_map.topography_enabled);

  _changed |= changed;

  return true;
}

Widget *
CreateTerrainDisplayConfigPanel()
{
  return new TerrainDisplayConfigPanel();
}
