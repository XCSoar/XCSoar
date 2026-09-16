// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroupedListTestDialog.hpp"
#include "ComboPicker.hpp"
#include "NumberEntry.hpp"
#include "UIGlobals.hpp"
#include "WidgetDialog.hpp"
#include "Form/Button.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "MapSettings.hpp"
#include "PageSettings.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Enum.hpp"
#include "Look/DialogLook.hpp"
#include "Look/MapLook.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Topography/TopographyStore.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/PaintWindow.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

#include <algorithm>
#include <functional>
#include <memory>

#include <utility>

#include <stdio.h>

using ItemOptions = GroupedListWidget::ItemOptions;

/**
 * Show one page of the menu.  Every page of the configuration is a
 * dialog of its own which fills the screen with a
 * #GroupedListWidget; a page which opens another one shows it on top
 * of itself, and closing it returns to the page below.
 */
static void
ShowMenuPage(const char *caption,
             const std::function<void(GroupedListWidget &)> &fill,
             std::unique_ptr<Widget> bottom_widget = {},
             unsigned bottom_widget_height_pt = 0,
             const std::function<void(WidgetDialog &,
                                      GroupedListWidget &)> &buttons = {}) noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  if (bottom_widget != nullptr)
    widget->SetBottomWidget(std::move(bottom_widget),
                            bottom_widget_height_pt);

  fill(*widget);

  GroupedListWidget &list = *widget;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, caption);
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);

  /* the buttons which act on the item under the cursor come after the
     list has been prepared: they follow the cursor */
  if (buttons)
    buttons(dialog, list);

  dialog.ShowModal();
}

/**
 * The item of a page which is not part of this demo: it shows what
 * the menu would offer, and says why nothing happens.
 */
static ItemOptions
Unavailable() noexcept
{
  return {.disabled = true, .disabled_badge_label = "demo"};
}

/**
 * The text which a #StaticEnumChoice list shows for one value.
 */
[[gnu::pure]]
static const char *
GetEnumText(const StaticEnumChoice *choices, unsigned value) noexcept
{
  for (auto i = choices; i->display_string != nullptr; ++i)
    if (i->id == value)
      return gettext(i->display_string);

  return "";
}

/**
 * Let the user pick one of the choices, like the configuration does
 * for a row which holds an enumeration.
 *
 * @return true if the value has changed
 */
static bool
PickEnum(const char *caption, const StaticEnumChoice *choices,
         unsigned &value, const char *help=nullptr) noexcept
{
  DataFieldEnum df;
  df.AddChoices(choices);
  df.SetValue(value);

  if (!ComboPicker(caption, df, help) || df.GetValue() == value)
    return false;

  value = df.GetValue();
  return true;
}

/* the terrain settings hold the two values as a byte, the dialog
   shows them as a percentage; the configuration panel converts them
   the same way */

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

/**
 * Let the user edit a percentage, like the configuration does for a
 * row which holds a number.
 */
static bool
PickPercent(const char *caption, short &value) noexcept
{
  int percent = ByteToPercent(value);

  if (!NumberEntryDialog(caption, percent, 3))
    return false;

  value = PercentToByte(std::clamp(percent, 0, 100));
  return true;
}

/* the values which the terrain page shows.  The demo works on a copy
   of the settings: it does not save anything, but the preview follows
   every change */
static TerrainRendererSettings terrain_settings;
static bool topography_enabled;

/**
 * The preview which the terrain page shows below its list; it is the
 * one of the configuration dialog.
 */
class TerrainPreviewWindow final : public PaintWindow {
  TerrainRenderer renderer;
  std::unique_ptr<TopographyRenderer> topography_renderer;

public:
  TerrainPreviewWindow(const RasterTerrain &terrain,
                       const TopographyStore *topography,
                       const TopographyLook &topography_look) noexcept
    :renderer(terrain)
  {
#ifdef ENABLE_OPENGL
    /* the preview is small: draw it at full resolution */
    renderer.SetQuantisationPixels(1);
#endif

    if (topography != nullptr)
      topography_renderer =
        std::make_unique<TopographyRenderer>(*topography, topography_look);
  }

  /** Take over the settings which the list has changed. */
  void Update() noexcept {
    renderer.SetSettings(terrain_settings);
    renderer.Flush();
    Invalidate();
  }

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.Clear(UIGlobals::GetDialogLook().background_color);

    const GlueMapWindow *map = UIGlobals::GetMap();
    if (map == nullptr)
      return;

    MapWindowProjection projection = map->VisibleProjection();
    if (!projection.IsValid())
      return;

    projection.SetScreenSize(canvas.GetSize());
    projection.SetScreenOrigin(canvas.GetRect().GetCenter());

    Angle sun_azimuth = Angle::Degrees(-45);
    if (terrain_settings.slope_shading == SlopeShading::SUN &&
        CommonInterface::Calculated().sun_data_available)
      sun_azimuth = CommonInterface::Calculated().sun_azimuth;

    renderer.Generate(projection, sun_azimuth);

#ifdef ENABLE_OPENGL
    /* the terrain renderer uses a texture which is larger than this
       window */
    const GLCanvasScissor scissor(canvas);
#endif

    renderer.Draw(canvas, projection);

    if (topography_enabled && topography_renderer != nullptr)
      topography_renderer->Draw(canvas, projection);
  }
};

class TerrainPreviewWidget final : public WindowWidget {
public:
  TerrainPreviewWindow &GetPreview() noexcept {
    return (TerrainPreviewWindow &)GetWindow();
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<TerrainPreviewWindow>(*data_components->terrain,
                                                    data_components->topography.get(),
                                                    UIGlobals::GetMapLook().topography);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

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

/**
 * Fill the list of the terrain page.  A value which the user has
 * changed is only shown after the list has been filled again, and the
 * items therefore call this function.
 */
static void
FillTerrainPage(GroupedListWidget &list, TerrainPreviewWidget *preview) noexcept
{
  list.Clear();

  /* the preview and the list show the same settings */
  const auto update = [&list, preview](){
    if (preview != nullptr)
      preview->GetPreview().Update();

    FillTerrainPage(list, preview);
    list.UpdateLayout();
  };

  list.AddGroup(nullptr);

  /* two settings which are nothing but a boolean: a switch says more
     than the words "On" and "Off" would */
  list.AddItem(_("Terrain Display"), [update](){
    terrain_settings.enable = !terrain_settings.enable;
    update();
  }, {.toggle = true, .checked = terrain_settings.enable,
      .help = _("Draw a digital elevation terrain on the map.")});

  list.AddItem(_("Topography display"), [update](){
    topography_enabled = !topography_enabled;
    update();
  }, {.toggle = true, .checked = topography_enabled,
      .help = _("Draw topographical features (roads, rivers, lakes etc.) "
                "on the map.")});

  list.AddGroup(_("Colors"));

  list.AddItem(_("Terrain colors"), [update](){
    unsigned value = terrain_settings.ramp;
    if (PickEnum(_("Terrain colors"), terrain_ramp_list, value)) {
      terrain_settings.ramp = value;
      update();
    }
  }, {.value = GetEnumText(terrain_ramp_list, terrain_settings.ramp),
      .chevron = true,
      .help = _("Defines the color ramp used in terrain rendering.")});

  list.AddItem(_("Slope shading"), [update](){
    unsigned value = (unsigned)terrain_settings.slope_shading;
    if (PickEnum(_("Slope shading"), slope_shading_list, value)) {
      terrain_settings.slope_shading = (SlopeShading)value;
      update();
    }
  }, {.value = GetEnumText(slope_shading_list,
                           (unsigned)terrain_settings.slope_shading),
      .chevron = true,
      .help = _("The terrain can be shaded among slopes to indicate either "
                "wind direction, sun position, a geographically fixed "
                "shading from North-West, or a screen-relative fixed "
                "shading from top left.")});

  char contrast[16], brightness[16];
  sprintf(contrast, "%d %%", ByteToPercent(terrain_settings.contrast));
  sprintf(brightness, "%d %%", ByteToPercent(terrain_settings.brightness));

  list.AddItem(_("Terrain contrast"), [update](){
    if (PickPercent(_("Terrain contrast"), terrain_settings.contrast))
      update();
  }, {.value = contrast, .chevron = true,
      .help = _("Defines the amount of Phong shading in the terrain "
                "rendering. Use large values to emphasise terrain slope, "
                "smaller values if flying in steep mountains.")});

  list.AddItem(_("Terrain brightness"), [update](){
    if (PickPercent(_("Terrain brightness"), terrain_settings.brightness))
      update();
  }, {.value = brightness, .chevron = true,
      .help = _("Defines the brightness (whiteness) of the terrain "
                "rendering. This controls the average illumination of "
                "the terrain.")});

  list.AddItem(_("Contours"), [update](){
    unsigned value = (unsigned)terrain_settings.contours;
    if (PickEnum(_("Contours"), contours_list, value)) {
      terrain_settings.contours = (Contours)value;
      update();
    }
  }, {.value = GetEnumText(contours_list,
                           (unsigned)terrain_settings.contours),
      .chevron = true,
      .help = _("Draw contour lines on the terrain. Contour mode controls "
                "density of contour lines.")});
}

/**
 * "Map Display / Terrain": every row of the configuration panel, as
 * an item which opens the same editor the panel would open, and the
 * preview of the panel below them.
 */
static void
ShowTerrainPage() noexcept
{
  const MapSettings &settings = CommonInterface::GetMapSettings();

  terrain_settings = settings.terrain;
  topography_enabled = settings.topography_enabled;

  /* the preview needs a terrain, and the demo may run without one */
  TerrainPreviewWidget *preview = nullptr;
  std::unique_ptr<Widget> bottom_widget;

  if (data_components != nullptr && data_components->terrain != nullptr) {
    auto p = std::make_unique<TerrainPreviewWidget>();
    preview = p.get();
    bottom_widget = std::move(p);
  }

  ShowMenuPage(_("Terrain"), [preview](GroupedListWidget &list){
    FillTerrainPage(list, preview);
  }, std::move(bottom_widget), 90);
}

static constexpr StaticEnumChoice page_main_list[] = {
  { PageLayout::Main::MAP, N_("Map") },
  { PageLayout::Main::MAP_NORTH_UP, N_("Map (north-up)") },
  { PageLayout::Main::FLARM_RADAR, N_("FLARM Radar") },
  { PageLayout::Main::THERMAL_ASSISTANT, N_("Thermal Assistant") },
  { PageLayout::Main::HORIZON, N_("Horizon") },
  nullptr
};

static constexpr StaticEnumChoice page_bottom_list[] = {
  { PageLayout::Bottom::NOTHING, N_("Nothing") },
  { PageLayout::Bottom::CROSS_SECTION, N_("Cross section") },
  { PageLayout::Bottom::WEATHER_CONTROLS, NC_("Setting", "Weather controls") },
  nullptr
};

/* the pages which the "Pages" page shows; the demo edits its own copy */
static PageSettings page_settings;

/**
 * The title of one page, as the page list of the configuration shows
 * it.
 */
static const char *
MakePageTitle(const PageLayout &page, std::span<char> buffer) noexcept
{
  return page.MakeTitle(CommonInterface::GetUISettings().info_boxes,
                        buffer, nullptr, true);
}

/**
 * Fill the list which edits one page; like the terrain page, the
 * items fill it again once they have changed a value.
 */
static void
FillPageEditor(GroupedListWidget &list, PageLayout &page) noexcept
{
  list.Clear();

  const auto update = [&list, &page](){
    FillPageEditor(list, page);
    list.UpdateLayout();
  };

  list.AddGroup(nullptr, {.footer = _("The flight display shows the pages "
                                      "in the order of this list.")});

  list.AddItem(_("Main area"), [&page, update](){
    unsigned value = (unsigned)page.main;
    if (PickEnum(_("Main area"), page_main_list, value)) {
      page.main = (PageLayout::Main)value;
      page.valid = true;
      update();
    }
  }, {.value = GetEnumText(page_main_list, (unsigned)page.main),
      .chevron = true,
      .help = _("Specifies what should be displayed in the main area.")});

  const auto &info_boxes = CommonInterface::GetUISettings().info_boxes;

  const char *info_box_value = !page.infobox_config.enabled
    ? _("None")
    : (page.infobox_config.auto_switch
       ? _("Auto")
       : gettext(info_boxes.panels[page.infobox_config.panel].name));

  list.AddItem(_("InfoBoxes"), [&page, update](){
    const auto &settings = CommonInterface::GetUISettings().info_boxes;

    DataFieldEnum df;
    df.AddChoice(0u, _("Auto"), nullptr,
                 _("Displays either the Circling, Cruise, or Final glide "
                   "InfoBoxes."));
    df.AddChoice(1u, _("None"), nullptr,
                 _("Show fullscreen (no InfoBoxes)"));

    for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; ++i)
      df.AddChoice(2 + i, gettext(settings.panels[i].name));

    df.SetValue(!page.infobox_config.enabled
                ? 1u
                : (page.infobox_config.auto_switch
                   ? 0u
                   : 2 + page.infobox_config.panel));

    if (!ComboPicker(_("InfoBoxes"), df))
      return;

    const unsigned value = df.GetValue();

    page.infobox_config.enabled = value != 1;
    page.infobox_config.auto_switch = value == 0;
    if (value >= 2)
      page.infobox_config.panel = value - 2;

    update();
  }, {.value = info_box_value, .chevron = true,
      .help = _("Specifies which InfoBoxes should be displayed on this "
                "page.")});

  list.AddItem(_("Bottom area"), [&page, update](){
    unsigned value = (unsigned)page.bottom;
    if (PickEnum(_("Bottom area"), page_bottom_list, value)) {
      page.bottom = (PageLayout::Bottom)value;
      update();
    }
  }, {.value = GetEnumText(page_bottom_list, (unsigned)page.bottom),
      .chevron = true,
      .help = _("Specifies what should be displayed below the main "
                "area.")});
}

/**
 * "Look / Pages": one page of the flight display, with the same three
 * choices which the configuration offers.
 */
static void
ShowPageEditor(unsigned i) noexcept
{
  PageLayout &page = page_settings.pages[i];

  char caption[64];
  ShowMenuPage(MakePageTitle(page, caption), [&page](GroupedListWidget &list){
    FillPageEditor(list, page);
  });
}

/**
 * Fill the list of the pages; an item shows what its page shows.
 */
static void
FillPagesPage(GroupedListWidget &list) noexcept
{
  list.Clear();

  list.AddGroup(nullptr,
                {.footer = _("Tap a page to see what it shows.  The demo "
                             "does not save the pages.")});

  for (unsigned i = 0; i < PageSettings::MAX_PAGES; ++i) {
    char caption[32], title[64];
    sprintf(caption, "%s %u", _("Page"), i + 1);

    list.AddItem(caption, [i, &list](){
      ShowPageEditor(i);

      /* the item shows what the page shows: fill the list again */
      FillPagesPage(list);
      list.UpdateLayout();
    }, {.value = MakePageTitle(page_settings.pages[i], title),
        .chevron = true});
  }
}

/**
 * Move the page under the cursor one step up or down, like the arrow
 * buttons of the configuration do.
 */
static void
MovePage(GroupedListWidget &list, int direction) noexcept
{
  const int i = list.GetCursorIndex();
  const int j = i + direction;
  if (i < 0 || j < 0 || j >= (int)PageSettings::MAX_PAGES)
    return;

  std::swap(page_settings.pages[i], page_settings.pages[j]);

  FillPagesPage(list);

  /* the cursor follows the page the user has moved */
  list.SetCursorIndex(j);
  list.UpdateLayout();
}

/**
 * "Look / Pages": the list of the pages which the flight display
 * shows, one item each.  Two buttons move the page under the cursor,
 * and the cursor decides whether they are available.
 */
static void
ShowPagesPage() noexcept
{
  page_settings = CommonInterface::GetUISettings().pages;

  ShowMenuPage(_("Pages"), FillPagesPage, {}, 0,
               [](WidgetDialog &dialog, GroupedListWidget &list){
    Button *const up = dialog.AddSymbolButton("^", [&list](){
      MovePage(list, -1);
    });

    Button *const down = dialog.AddSymbolButton("v", [&list](){
      MovePage(list, 1);
    });

    const auto update = [up, down](int index){
      up->SetEnabled(index > 0);
      down->SetEnabled(index >= 0 &&
                       index + 1 < (int)PageSettings::MAX_PAGES);
    };

    list.SetCursorCallback([update](int index){
      update((int)index);
    });

    update(list.GetCursorIndex());
  });
}

/**
 * "Map Display": the pages which the configuration shows in this
 * group.
 */
static void
ShowMapDisplayPage() noexcept
{
  ShowMenuPage(_("Map Display"), [](GroupedListWidget &list){
    list.AddGroup(nullptr);
    list.AddItem(_("Orientation"), Unavailable());
    list.AddItem(_("Elements"), Unavailable());
    list.AddItem(_("Waypoints"), Unavailable());
    list.AddItem(_("Terrain"), ShowTerrainPage, {.chevron = true});
    list.AddItem(_("Airspace"), Unavailable());
  });
}

/**
 * "Look": the pages which the configuration shows in this group.
 */
static void
ShowLookPage() noexcept
{
  ShowMenuPage(_("Look"), [](GroupedListWidget &list){
    list.AddGroup(nullptr);
    list.AddItem(_("Language, Input"), Unavailable());
    list.AddItem(_("Display"), Unavailable());
    list.AddItem(_("Layout"), Unavailable());
    list.AddItem(_("Pages"), ShowPagesPage, {.chevron = true});
    list.AddItem(_("InfoBox Sets"), Unavailable());
  });
}

void
ShowGroupedListMenuDialog() noexcept
try {
  ShowMenuPage(_("Configuration"), [](GroupedListWidget &list){
    list.AddHero(_("Configuration"),
                   _("The configuration of XCSoar as a list of pages, one "
                     "page opening the next.  Only Map Display and Look "
                     "lead somewhere in this demo."));

    list.AddGroup(nullptr,
                  {.footer = _("The pages which are not available here are "
                               "the ones of the configuration dialog, which "
                               "this demo does not replace.")});

    list.AddItem(_("Site Files"), Unavailable());
    list.AddItem(_("Map Display"), ShowMapDisplayPage, {.chevron = true});
    list.AddItem(_("Glide Computer"), Unavailable());
    list.AddItem(_("Gauges"), Unavailable());
    list.AddItem(_("Task Defaults"), Unavailable());
    list.AddItem(_("Look"), ShowLookPage, {.chevron = true});
    list.AddItem(_("Weather"), Unavailable());
    list.AddItem(_("Setup"), Unavailable());
  });
} catch (...) {
  LogError(std::current_exception(), "Grouped List Menu");
}
