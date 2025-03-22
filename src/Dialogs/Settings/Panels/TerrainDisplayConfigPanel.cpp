// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainDisplayConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Interface.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
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

  void OnPaint(Canvas &canvas) noexcept override;
};

class TerrainDisplayConfigPanel final
  : public RowFormWidget, DataFieldListener {

  bool have_terrain_preview;

protected:
  TerrainRendererSettings terrain_settings;

public:
  TerrainDisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void ShowTerrainControls();

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

protected:
  void UpdateTerrainPreview();

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
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
  if (have_terrain_preview)
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
    GetValueEnum(TerrainSlopeShading);
  terrain_settings.contrast = PercentToByte(GetValueInteger(TerrainContrast));
  terrain_settings.brightness =
    PercentToByte(GetValueInteger(TerrainBrightness));
  terrain_settings.ramp = GetValueEnum(TerrainColors);
  terrain_settings.contours = (Contours)
    GetValueEnum(TerrainContours);

  // Invalidate terrain preview
  if (have_terrain_preview)
    ((TerrainPreviewWindow &)GetRow(TerrainPreview)).SetSettings(terrain_settings);
}

void
TerrainDisplayConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(EnableTerrain, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    terrain_settings.enable = dfb.GetValue();
    ShowTerrainControls();
  } else {
    UpdateTerrainPreview();
  }
}

void
TerrainPreviewWindow::OnPaint(Canvas &canvas) noexcept
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
  projection.SetScreenOrigin(canvas.GetRect().GetCenter());

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
TerrainDisplayConfigPanel::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
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
    {15, N_("High Contrast"), },
    {16, N_("High Contrast low lands"), },
    {17, N_("Very low lands"), },
    nullptr
  };

  AddEnum(_("Terrain colors"),
          _("Defines the color ramp used in terrain rendering."),
          terrain_ramp_list, terrain.ramp);
  GetDataField(TerrainColors).SetListener(this);

  static constexpr StaticEnumChoice slope_shading_list[] = {
    { SlopeShading::OFF, N_("Off"), },
    { SlopeShading::FIXED, N_("Fixed"), },
    { SlopeShading::SUN, N_("Sun"), },
    { SlopeShading::WIND, N_("Wind"), },
    nullptr
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
    { Contours::OFF, N_("Off"), },
    { Contours::ON, N_("On"), },
    nullptr
  };

  AddEnum(_("Contours"),
          _("If enabled, draws contour lines on the terrain."),
          contours_list, (unsigned)terrain.contours);
  GetDataField(TerrainContours).SetListener(this);
  SetExpertRow(TerrainContours);

  have_terrain_preview = data_components->terrain != nullptr;
  if (have_terrain_preview) {
    WindowStyle style;
    style.Border();

    auto preview = std::make_unique<TerrainPreviewWindow>(*data_components->terrain);
    preview->Create((ContainerWindow &)GetWindow(), {0, 0, 100, 100}, style);
    AddRemaining(std::move(preview));
  }

  terrain_settings = terrain;
  ShowTerrainControls();
  UpdateTerrainPreview();
}

bool
TerrainDisplayConfigPanel::Save(bool &_changed) noexcept
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

std::unique_ptr<Widget>
CreateTerrainDisplayConfigPanel()
{
  return std::make_unique<TerrainDisplayConfigPanel>();
}
