// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainDisplayConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "ActionInterface.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/MapLook.hpp"
#include "MapSettings.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Message.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Topography/TopographyStore.hpp"
#include "UIGlobals.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/window/SingleWindow.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

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

static constexpr StaticEnumChoice slope_shading_list[] = {
  { SlopeShading::OFF, N_("Off"), },
  { SlopeShading::FIXED, NC_("Setting", "Fixed (North-West)"), },
  { SlopeShading::SUN, N_("Sun"), },
  { SlopeShading::WIND, N_("Wind"), },
  { SlopeShading::TOP_LEFT, NC_("Setting", "Fixed (Top Left)"), },
  nullptr
};

static constexpr StaticEnumChoice contours_list[] = {
  { Contours::OFF, N_("Off"), NC_("Setting", "No contour lines"), },
  { Contours::MOUNTAINS, NC_("Setting", "Mountains"),
    N_("For steep mountain terrain, 256m minimum spacing"), },
  { Contours::HIGHLANDS, NC_("Setting", "Highlands"),
    N_("Medium density, with 64m minimum spacing"), },
  { Contours::LOWLANDS, NC_("Setting", "Lowlands"),
    N_("More line density for gentler slopes. 16m minimum spacing"), },
  { Contours::SUPERFINE, NC_("Setting", "Superfine"),
    N_("Maximum density contour lines down to 8m spacing"), },
  { Contours::FIXED_256, NC_("Setting", "Fixed 256m"),
    N_("Fixed 256m spacing, no zoom dependence"), },
  { Contours::FIXED_128, NC_("Setting", "Fixed 128m"),
    N_("Fixed 128m spacing, no zoom dependence"), },
  { Contours::FIXED_64, NC_("Setting", "Fixed 64m"),
    N_("Fixed 64m spacing, no zoom dependence"), },
  nullptr
};

/* the settings hold contrast and brightness as a byte, the page shows
   them as a percentage */

[[gnu::const]]
static short
ByteToPercent(short byte) noexcept
{
  return (byte * 200 + 100) / 510;
}

[[gnu::const]]
static short
PercentToByte(short percent) noexcept
{
  return (percent * 510 + 255) / 200;
}

namespace {

/**
 * A piece of the map, drawn with the settings of the page as they
 * are now.
 */
class TerrainPreviewWindow final : public PaintWindow {
  TerrainRenderer renderer;
  std::unique_ptr<TopographyRenderer> topography_renderer;
  bool topography_enabled = false;

public:
  TerrainPreviewWindow(const RasterTerrain &terrain,
                       const TopographyStore *topography,
                       const TopographyLook &topography_look) noexcept
    :renderer(terrain)
  {
#ifdef ENABLE_OPENGL
    /* always render at full resolution in the preview; the default
       idle-based quantisation would produce a blocky image while the
       user interacts with the dialog */
    renderer.SetQuantisationPixels(1);
#endif

    if (topography != nullptr)
      topography_renderer =
        std::make_unique<TopographyRenderer>(*topography, topography_look);
  }

  void SetSettings(const TerrainRendererSettings &settings,
                   bool _topography_enabled) noexcept {
    renderer.SetSettings(settings);
    renderer.Flush();
    topography_enabled = _topography_enabled;
    Invalidate();
  }

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};

void
TerrainPreviewWindow::OnPaint(Canvas &canvas) noexcept
{
  /* the map is white where it shows no terrain */
  canvas.ClearWhite();

  const GlueMapWindow *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  MapWindowProjection projection = map->VisibleProjection();
  if (!projection.IsValid())
    /* TODO: initialise projection to middle of map instead of bailing
       out */
    return;

  projection.SetScreenSize(canvas.GetSize());
  projection.SetScreenOrigin(canvas.GetRect().GetCenter());

  /* the bounds are cached: the terrain renderer would draw the area
     which the map shows, not the one the preview does */
  projection.UpdateScreenBounds();

#ifdef ENABLE_OPENGL
  /* enable clipping because the OpenGL renderers of the terrain and
     the topography draw beyond the window */
  GLCanvasScissor scissor(canvas);
#endif

  if (renderer.GetSettings().enable) {
    Angle sun_azimuth(Angle::Degrees(-45));
    if (renderer.GetSettings().slope_shading == SlopeShading::SUN &&
        CommonInterface::Calculated().sun_data_available)
      sun_azimuth = CommonInterface::Calculated().sun_azimuth;

    renderer.Generate(projection, sun_azimuth);
    renderer.Draw(canvas, projection);
  }

  if (topography_enabled && topography_renderer)
    topography_renderer->Draw(canvas, projection);
}

/**
 * The preview below the list.  It asks for a third of the screen,
 * which shows enough of the map to judge the colours, and the list
 * keeps half of the page in any case; on a screen so small that this
 * is less than three rows, it takes three rows.
 */
class TerrainPreviewWidget final : public WindowWidget {
public:
  /** Draw the map with these settings from now on. */
  void SetSettings(const TerrainRendererSettings &settings,
                   bool topography_enabled) noexcept {
    if (IsDefined())
      ((TerrainPreviewWindow &)GetWindow()).SetSettings(settings,
                                                        topography_enabled);
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return {0u, Layout::GetMaximumControlHeight() * 3};
  }

  PixelSize GetMaximumSize() const noexcept override {
    return {0u, UIGlobals::GetMainWindow().GetSize().height / 3};
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();
    style.Border();

    auto w = std::make_unique<TerrainPreviewWindow>(*data_components->terrain,
                                                    data_components->topography.get(),
                                                    UIGlobals::GetMapLook().topography);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

/**
 * The terrain and the topography of the map, with a preview of the
 * map below the list.
 */
class TerrainDisplayConfigPanel final : public ConfigListPanel {
  /** the values of the page; the preview follows them */
  TerrainRendererSettings terrain_settings;

  /**
   * The values when the page was opened, so that Save() does not
   * write unchanged defaults into a profile which lacks the keys
   * (#1793).
   */
  TerrainRendererSettings initial_terrain_settings;

  bool topography_enabled, initial_topography_enabled;

  /** contrast and brightness as the page shows them */
  int contrast, brightness;

  /** the preview below the list; nullptr without a terrain */
  TerrainPreviewWidget *preview = nullptr;

public:
  TerrainDisplayConfigPanel() noexcept {
    if (data_components->terrain != nullptr) {
      auto p = std::make_unique<TerrainPreviewWidget>();
      preview = p.get();
      SetBottomWidget(std::move(p));
    }
  }

private:
  /**
   * Add the switch of the terrain or the topography: it acts on the
   * map right away, so that the pilot sees what it does.
   */
  void AddMapToggleItem(const char *caption, const char *help,
                        bool &value, const char *shown,
                        const char *hidden) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;
  void Refresh() noexcept override;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
TerrainDisplayConfigPanel::AddMapToggleItem(const char *caption,
                                            const char *help, bool &value,
                                            const char *shown,
                                            const char *hidden) noexcept
{
  const unsigned item = GetItemCount();

  AddItem(caption, [this, item, &value, shown, hidden](){
    value = IsItemChecked(item);

    MapSettings &settings_map = CommonInterface::SetMapSettings();
    settings_map.terrain.enable = terrain_settings.enable;
    settings_map.topography_enabled = topography_enabled;
    Message::AddMessage(value ? shown : hidden);
    ActionInterface::SendMapSettings(true);

    Refresh();
  }, {.toggle = true, .checked = value, .help = help});
}

void
TerrainDisplayConfigPanel::LoadSettings() noexcept
{
  const MapSettings &settings_map = CommonInterface::GetMapSettings();

  terrain_settings = settings_map.terrain;
  topography_enabled = initial_topography_enabled =
    settings_map.topography_enabled;

  contrast = ByteToPercent(terrain_settings.contrast);
  brightness = ByteToPercent(terrain_settings.brightness);

  /* the conversion to a percentage and back is lossy for some values:
     compare against what the page would save unchanged */
  terrain_settings.contrast = PercentToByte(contrast);
  terrain_settings.brightness = PercentToByte(brightness);
  initial_terrain_settings = terrain_settings;
}

void
TerrainDisplayConfigPanel::Fill() noexcept
{
  AddGroup();

  AddMapToggleItem(_("Terrain Display"),
                   _("Draw a digital elevation terrain on the map."),
                   terrain_settings.enable,
                   _("Terrain shown"), _("Terrain hidden"));

  AddMapToggleItem(_("Topography display"),
                   _("Draw topographical features (roads, rivers, lakes etc.) on the map."),
                   topography_enabled,
                   _("Topography shown"), _("Topography hidden"));

  if (!terrain_settings.enable)
    return;

  AddGroup(_("Terrain"));

  AddEnumItem(_("Terrain colors"),
              _("Defines the color ramp used in terrain rendering."),
              terrain_ramp_list, terrain_settings.ramp);

  if (IsExpert()) {
    AddEnumItem(_("Slope shading"),
                _("The terrain can be shaded among slopes to indicate either "
                  "wind direction, sun position, a geographically fixed shading from "
                  "North-West, or a screen-relative fixed shading from top left."),
                slope_shading_list, terrain_settings.slope_shading);

    AddPercentItem(_("Terrain contrast"),
                   _("Defines the amount of Phong shading in the terrain rendering. Use large values to emphasise terrain slope, smaller values if flying in steep mountains."),
                   0, 100, 5, contrast);

    AddPercentItem(_("Terrain brightness"),
                   _("Defines the brightness (whiteness) of the terrain rendering. This controls the average illumination of the terrain."),
                   0, 100, 5, brightness);

    AddEnumItem(_("Contours"),
                _("Draw contour lines on the terrain. Contour mode "
                  "controls density of contour lines."),
                contours_list, terrain_settings.contours);
  }
}

void
TerrainDisplayConfigPanel::Refresh() noexcept
{
  terrain_settings.contrast = PercentToByte(contrast);
  terrain_settings.brightness = PercentToByte(brightness);

  ConfigListPanel::Refresh();

  if (preview != nullptr)
    preview->SetSettings(terrain_settings, topography_enabled);
}

void
TerrainDisplayConfigPanel::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
{
  ConfigListPanel::Prepare(parent, rc);

  /* the preview exists only now */
  if (preview != nullptr)
    preview->SetSettings(terrain_settings, topography_enabled);
}

bool
TerrainDisplayConfigPanel::Save(bool &_changed) noexcept
{
  MapSettings &settings_map = CommonInterface::SetMapSettings();

  bool changed = false;

  /* the map has the values already, the switches have applied them;
     the profile gets them only if they differ from the ones the page
     was opened with, so that missing defaults stay absent (#1793) */
  settings_map.terrain = terrain_settings;
  if (terrain_settings != initial_terrain_settings) {
    Profile::Set(ProfileKeys::DrawTerrain, terrain_settings.enable);
    Profile::Set(ProfileKeys::TerrainContrast, terrain_settings.contrast);
    Profile::Set(ProfileKeys::TerrainBrightness, terrain_settings.brightness);
    Profile::Set(ProfileKeys::TerrainRamp, terrain_settings.ramp);
    Profile::SetEnum(ProfileKeys::SlopeShadingType,
                     terrain_settings.slope_shading);
    Profile::SetEnum(ProfileKeys::TerrainContours, terrain_settings.contours);
    changed = true;
  }

  settings_map.topography_enabled = topography_enabled;
  if (topography_enabled != initial_topography_enabled) {
    Profile::Set(ProfileKeys::DrawTopography, topography_enabled);
    changed = true;
  }

  _changed |= changed;

  return true;
}

} // anonymous namespace

std::unique_ptr<Widget>
CreateTerrainDisplayConfigPanel()
{
  return std::make_unique<TerrainDisplayConfigPanel>();
}
