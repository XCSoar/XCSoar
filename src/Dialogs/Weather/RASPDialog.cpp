// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Dialogs/Settings/Panels/ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/File.hpp"
#include "Widget/WindowWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/Settings.hpp"
#include "WeatherOverlayDraft.hpp"
#include "Weather/Rasp/RaspStyle.hpp"
#include "Weather/Rasp/ColorMap.hpp"
#include "Weather/Rasp/RaspRenderer.hpp"
#include "Terrain/RasterRenderer.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Math/Angle.hpp"
#include "Units/Units.hpp"
#include "Units/System.hpp"
#include "Units/Descriptor.hpp"
#include "Units/Unit.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/ConstantAlpha.hpp"
#endif
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Interface.hpp"
#include "PageSettings.hpp"
#include "Repository/FileType.hpp"
#include "UISettings.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "UIState.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"
#include "net/http/Features.hpp"
#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#include "net/http/DownloadManager.hpp"
#endif

#include <fmt/format.h>
#include <algorithm>
#include <span>

class RaspColorbarWindow : public PaintWindow {
  const DialogLook &look;
  const RaspStyle *style = nullptr;
  ContourDensity contour_density = ContourDensity::OFF;

public:
  explicit RaspColorbarWindow(const DialogLook &_look) noexcept
    :look(_look) {}

  void SetStyle(const RaspStyle *_style,
                ContourDensity _contour_density) noexcept {
    style = _style;
    contour_density = _contour_density;
    Invalidate();
  }

  void OnPaint(Canvas &canvas) noexcept override;
};

void
RaspColorbarWindow::OnPaint(Canvas &canvas) noexcept
{
  const auto rc = canvas.GetRect();

  if (style == nullptr) {
    canvas.Clear(look.background_color);
    return;
  }

  const unsigned height_scale = style->height_scale;

  // Build the color table using the same code path
  // as the map renderer
  auto materialized =
    MaterializeColorRamp(style->color_map,
                         style->color_map_alpha,
                         style->scale, style->offset,
                         height_scale, style->do_water);
  auto ramp = materialized.GetColorRamp();

  // Gate alpha on the backend's per-pixel source-alpha capability, matching
  // the map renderer, so the preview never diverges from the actual map.
  const bool use_alpha = ramp.has_alpha && HaveBitmapSourceAlpha();
  const auto &map = use_alpha
    ? style->color_map_alpha : style->color_map;
  const float min_v = map.points[0].value;
  const float max_v = map.points[map.num_points - 1].value;

  // Compute rendering-domain bounds from the physical
  // color map range
  const int16_t min_h = (int16_t)std::clamp(
    (int)(min_v * style->scale + style->offset),
    0, (int)INT16_MAX);
  const int16_t max_h = (int16_t)std::clamp(
    (int)(max_v * style->scale + style->offset),
    0, (int)INT16_MAX);

  RasterRenderer renderer;
  if (use_alpha)
    renderer.PrepareColorTableAlpha(
      &ramp, style->do_water,
      height_scale, RASP_INTERP_LEVELS);
  else
    renderer.PrepareColorTable(
      &ramp, style->do_water,
      height_scale, RASP_INTERP_LEVELS);

  canvas.Select(look.text_font);
  const unsigned font_h = canvas.CalcTextSize("0").height;
  const int bar_bottom = rc.bottom - font_h - Layout::Scale(2);
  const unsigned width = rc.right - rc.left;
  const unsigned bar_height = std::max(0, bar_bottom - rc.top);

  canvas.DrawFilledRectangle(PixelRect{rc.left, bar_bottom, rc.right, rc.bottom},
                             look.background_color);

  if (width == 0 || bar_height == 0)
    return;

  // Fill a synthetic height matrix with a horizontal
  // gradient and render through the full pipeline
  renderer.FillGradient({width, bar_height},
                        min_h, max_h);
  const unsigned contour_spacing =
    ContourSpacing(contour_density, height_scale);
  renderer.GenerateImage(false, height_scale,
                         0, 0, Angle::Zero(), contour_spacing);

  if (use_alpha) {
    // Draw checkerboard background for alpha styles
    constexpr unsigned CHECK_SQUARES_Y = 7;
    const unsigned check_squares_x = std::max(1u, CHECK_SQUARES_Y * width / bar_height);
    const unsigned check_size_x = width / check_squares_x + 1;
    const unsigned check_size_y = bar_height / CHECK_SQUARES_Y + 1;
    const Color light_color(230, 230, 230);
    const Color dark_color(26, 26, 26);

    for (unsigned iy = 0; iy < CHECK_SQUARES_Y; iy++) {
         unsigned y = iy * bar_height / CHECK_SQUARES_Y;
      for (unsigned ix = 0; ix < check_squares_x; ix++) {
        unsigned x = ix * width / check_squares_x;
        const bool light = (ix + iy + 1) % 2 == 0;
        canvas.DrawFilledRectangle(
          PixelRect{(int)x, (int)y,
            std::min((int)(x + check_size_x),
                     (int)width),
            std::min((int)(y + check_size_y),
                     (int)bar_height)},
          light ? light_color : dark_color);
      }
    }
  }

#ifdef ENABLE_OPENGL
  const ScopeTextureConstantAlpha blend(use_alpha, 1.0f);
#endif
  renderer.GetImage().StretchTo(
    PixelSize{width, bar_height}, canvas,
    PixelSize{width, bar_height}, false, use_alpha);

  // Draw min/max text labels, converted to the user's units
  const Unit unit = style->unit_group == UnitGroup::NONE
    ? Unit::UNDEFINED
    : Units::GetUserUnitByGroup(style->unit_group);
  const char *const unit_name = unit == Unit::UNDEFINED
    ? nullptr : Units::GetUnitName(unit);

  const RaspStyle &s = *style;
  auto fmt_value = [&s, unit, unit_name](float v) -> std::string {
    const double value = unit == Unit::UNDEFINED
      ? (double)v
      : Units::ToUserUnit(s.ToSystemValue(v), unit);

    std::string text = (value >= 100.0 || value <= -100.0)
      ? fmt::format("{:.0f}", value)
      : fmt::format("{:.1f}", value);

    if (unit_name != nullptr)
      text += fmt::format(" {}", unit_name);

    return text;
  };

  canvas.SetTextColor(look.text_color);
  canvas.SetBackgroundTransparent();

  try {
    const auto min_text = fmt_value(min_v);
    const auto max_text = fmt_value(max_v);
    const int text_offset = Layout::Scale(1);

    canvas.DrawText({rc.left + text_offset, bar_bottom + text_offset},
                    min_text);
    const auto max_size = canvas.CalcTextSize(max_text);
    canvas.DrawText({rc.right - (int)max_size.width - text_offset,
                     bar_bottom + text_offset},
                    max_text);
  } catch (...) {
    // Suppress formatting/allocation failures; colorbar rendering is preserved
  }
}

/** The colorbar of a layer as a view inside the list. */
class RaspColorbarWidget final : public WindowWidget {
  const RaspStyle *const style;
  const ContourDensity contour_density;

public:
  RaspColorbarWidget(const RaspStyle *_style,
                     ContourDensity _contour_density) noexcept
    :style(_style), contour_density(_contour_density) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle window_style;
    window_style.Hide();
    window_style.Border();

    auto w = std::make_unique<RaspColorbarWindow>(UIGlobals::GetDialogLook());
    w->Create(parent, rc, window_style);
    w->SetStyle(style, contour_density);
    SetWindow(std::move(w));
  }
};

static constexpr StaticEnumChoice contour_density_list[] = {
  { ContourDensity::OFF,       N_("Off") },
  { ContourDensity::WIDE,      N_("Wide") },
  { ContourDensity::REGULAR,   N_("Regular") },
  { ContourDensity::FINE,      N_("Fine") },
  { ContourDensity::SUPERFINE, N_("Superfine") },
  nullptr,
};

static_assert(ARRAY_SIZE(contour_density_list) ==
              unsigned(ContourDensity::COUNT) + 1,
              "contour_density_list must match ContourDensity::COUNT");

/**
 * The RASP forecast: the file and how the overlay is drawn, which
 * are the same for every page, and the layer and the time of the
 * page the map shows now.
 */
class RaspSettingsWidget final : public ConfigListPanel {
  std::shared_ptr<RaspStore> rasp;

  FileDataField file;

#ifdef HAVE_DOWNLOAD_MANAGER
  bool auto_update;
#endif

  int opacity;
  ContourDensity contour_density;

  /** the layer and the time of the current page */
  WeatherOverlayDraft::State overlay;

public:
  explicit RaspSettingsWidget(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :rasp(std::move(_rasp)) {}

private:
  void ReloadRasp() noexcept;
  void PickRaspFile() noexcept;

  void AddFileItems() noexcept;
  void AddPageItems() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RaspSettingsWidget::LoadSettings() noexcept
{
  file.SetFileType(FileType::RASP);
  file.AddNull();
  file.ScanMultiplePatterns(GetFileTypePatterns(FileType::RASP));
  file.Sort(FileDataField::SortOrder::ASCENDING, true);

  if (const auto path = Profile::GetPath(ProfileKeys::RaspFile);
      path != nullptr)
    file.SetValue(path);

#ifdef HAVE_DOWNLOAD_MANAGER
  auto_update =
    CommonInterface::GetComputerSettings().weather.rasp.auto_update;
#endif

  const MapSettings &map_settings = CommonInterface::GetMapSettings();
  opacity = map_settings.rasp_layer_opacity;
  contour_density = map_settings.rasp_contour_density;

  overlay.Load(PageLayout::Overlay::RASP);
}

void
RaspSettingsWidget::ReloadRasp() noexcept
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  Profile::Save();
  overlay.Load(PageLayout::Overlay::RASP);
  WeatherMapOverlay::RefreshControlsLabels();
}

void
RaspSettingsWidget::PickRaspFile() noexcept
{
  PickFile(_("File"), nullptr, file);

  if (Profile::SetPath(ProfileKeys::RaspFile, file.GetValue()))
    ReloadRasp();

  Refresh();
}

void
RaspSettingsWidget::AddFileItems() noexcept
{
  AddGroup(_("File"));

  ItemOptions file_options{.value_size = TextSize::SMALL,
                           .value_all_lines = true,
                           .chevron = true};

  const char *name = file.GetAsDisplayString();
  if (*name != '\0')
    file_options.value = name;
  else
    file_options.badge = C_("Badge", "none");

  AddItem(_("File"), [this](){ PickRaspFile(); }, file_options);

  /* when the file was written */
  StaticString<32> modified;
  modified.clear();

  if (rasp != nullptr) {
    const BrokenDateTime time = rasp->GetFileModifiedTime();
    if (time.IsPlausible())
      modified.Format("%04u-%02u-%02u %02u:%02u",
                      time.year, time.month, time.day,
                      time.hour, time.minute);
  }

  if (modified.empty())
    modified = _("Unknown");

  AddItem(C_("Status", "Modified"),
          {.value = modified.c_str(),
           .help = _("Local date and time of the selected RASP file.")});

#ifdef HAVE_DOWNLOAD_MANAGER
  const bool can_download = Net::DownloadManager::IsAvailable();

  /* the switch acts right away: the download it allows may start
     now, and the weather dialog closes without a Save() */
  const unsigned auto_update_item = GetItemCount();
  AddItem(C_("Setting", "Auto update"), [this, auto_update_item](){
    auto_update = IsItemChecked(auto_update_item);

    auto &weather = CommonInterface::SetComputerSettings().weather;
    if (Profile::Update(ProfileKeys::RaspAutoUpdate,
                        weather.rasp.auto_update, auto_update))
      Profile::Save();

    if (auto_update)
      RequestConfiguredRaspUpdateIfOutOfDate();
  }, {.toggle = true,
      .checked = auto_update,
      .help = _("Automatically download a newer RASP file when the "
                "configured forecast is missing or out of date."),
      .disabled = !can_download});

  /* a download by hand needs a file to update, whether the switch is
     on or not */
  AddButton(_("Update"), [](){ RequestConfiguredRaspUpdate(); },
            {.disabled = !can_download || file.GetValue().empty()});
#endif
}

void
RaspSettingsWidget::AddPageItems() noexcept
{
  const auto &ui_state = CommonInterface::GetUIState();
  const auto &ui_settings = CommonInterface::GetUISettings();
  const unsigned page_index = ui_state.pages.current_index;
  const PageLayout &page = ui_settings.pages.pages[page_index];

  /* the title of the page the map shows now; without the RASP field,
     which is chosen right here */
  StaticString<64> title_buffer;
  const char *title =
    page.MakeTitle(ui_settings.info_boxes,
                   std::span{title_buffer.data(), title_buffer.capacity()});

  StaticString<128> caption;
  caption.Format("%s %u: %s", _("Page"), page_index + 1, title);

  AddGroup(caption, {
    .footer = _("The layer and the forecast time of the page the map "
                "shows now.  Apply to page stores them on that page; "
                "Add page makes a new page with them."),
  });

  const bool has_fields = rasp != nullptr && rasp->GetItemCount() > 0;
  PageLayout &draft = overlay.draft;

  const char *layer = _("None");
  if (has_fields && draft.rasp_field >= 0 &&
      unsigned(draft.rasp_field) < rasp->GetItemCount()) {
    const auto &item = rasp->GetItemInfo(draft.rasp_field);
    layer = item.label != nullptr ? gettext(item.label) : item.name.c_str();
  }

  AddItem(C_("Weather control", "Layer"), [this](){
    if (Rasp::PickField(C_("Weather control", "Layer"),
                        _("RASP weather layer for the current map page. "
                          "Use Apply to page to commit changes."),
                        *rasp, overlay.draft.rasp_field, true))
      Refresh();
  }, {.value = layer, .chevron = true, .disabled = !has_fields});

  StaticString<64> time;
  Rasp::FormatTimeLabelForPage(time, draft);

  AddItem(C_("Weather control", "Time"), [this](){
    if (Rasp::EditTimeOnLayout(overlay.draft))
      Refresh();
  }, {.value = time.c_str(),
      .chevron = true,
      .disabled = !Rasp::IsFieldTimeSelectable(draft.rasp_field)});

  /* the colors of the layer, as the map draws them */
  if (has_fields && draft.rasp_field >= 0 &&
      unsigned(draft.rasp_field) < rasp->GetItemCount()) {
    const auto &item = rasp->GetItemInfo(draft.rasp_field);
    AddWidgetGroup(nullptr,
                   std::make_unique<RaspColorbarWidget>(
                     &LookupWeatherTerrainStyle(item.name),
                     contour_density),
                   40);
  }

  AddButtonRow({
    {C_("Button", "Apply to page"), [this](){
      if (overlay.ApplyIfDirty())
        Refresh();
    }, !overlay.IsDirty()},
    {C_("Button", "Add page"), [this](){
      overlay.AddPage(nullptr, nullptr);
      Refresh();
    }, !overlay.CanAddPage()},
  });

  AddGroup();
  AddButton(C_("Button", "Pages setup"), [this](){
    WeatherOverlayDraft::OpenPagesConfig();
    overlay.Load(PageLayout::Overlay::RASP);
    Refresh();
  });
}

void
RaspSettingsWidget::Fill() noexcept
{
  AddFileItems();

  /* how the overlay is drawn, on every page */
  AddGroup(C_("Setting", "Map overlay"));

  AddPercentItem(_("Overlay opacity"),
                 /* xgettext:no-c-format */
                 _("Sets the opacity of the RASP weather overlay on the map.  "
                   "0% is fully transparent, 100% is fully opaque."),
                 0, 100, 10, opacity);

  AddEnumItem(_("Contours"),
              _("Draws contour lines onto the RASP weather "
                "overlay.  Denser settings draw more lines; "
                "\"Off\" disables the contour lines."),
              contour_density_list, contour_density);

  AddPageItems();
}

void
RaspSettingsWidget::Show(const PixelRect &rc) noexcept
{
  /* the map may show another page than the last time */
  overlay.Load(PageLayout::Overlay::RASP);
  Refresh();

  ConfigListPanel::Show(rc);
}

bool
RaspSettingsWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

#ifdef HAVE_DOWNLOAD_MANAGER
  auto &weather = CommonInterface::SetComputerSettings().weather;
  changed |= Profile::Update(ProfileKeys::RaspAutoUpdate,
                             weather.rasp.auto_update, auto_update);
#endif

  MapSettings &map_settings = CommonInterface::SetMapSettings();

  const bool contours_changed =
    Profile::Update(ProfileKeys::RaspContours,
                    map_settings.rasp_contour_density, contour_density);
  const bool opacity_changed =
    Profile::Update(ProfileKeys::RaspLayerOpacity,
                    map_settings.rasp_layer_opacity, uint8_t(opacity));

  if (contours_changed || opacity_changed) {
    /* the redraw is triggered by the SendUIState() call below */
    ActionInterface::SendMapSettings(false);
    changed = true;
  }

  if (changed) {
    /* This dialog can be shown standalone (via the weather dialog),
       where nothing else writes the profile to disk, so persist here. */
    Profile::Save();
    ActionInterface::SendUIState(true);
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RaspSettingsWidget>(std::move(rasp));
}
