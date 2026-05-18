// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "LogFile.hpp"

#include <ctime>
#include <chrono>

namespace {

constexpr unsigned XCTHERM_MODEL_CH = 0;
constexpr unsigned XCTHERM_MODEL_UK = 1;

struct LayerInfo {
  const char *label;       // display text
  const char *file_suffix; // e.g. "5000amsl" or "100agl"
  unsigned value;          // altitude value in meters
  bool is_agl;             // true = AGL, false = AMSL
};

static constexpr LayerInfo CH_LAYERS[] = {
  { "Vertical wind 1500 m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 3000 m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4000 m AMSL", "4000amsl", 4000, false },
  { "Vertical wind 5000 m AMSL", "5000amsl", 5000, false },
  { "Vertical wind 6000 m AMSL", "6000amsl", 6000, false },
  { "Vertical wind 7000 m AMSL", "7000amsl", 7000, false },
  { "Vertical wind 8000 m AMSL", "8000amsl", 8000, false },
  { "Vertical wind 100 m AGL",   "100agl",   100,  true },
  { "Vertical wind 400 m AGL",   "400agl",   400,  true },
};

static constexpr LayerInfo UK_LAYERS[] = {
  { "Vertical wind 1000 m AMSL", "1000amsl", 1000, false },
  { "Vertical wind 1500 m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 2500 m AMSL", "2500amsl", 2500, false },
  { "Vertical wind 3000 m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4200 m AMSL", "4200amsl", 4200, false },
  { "Vertical wind 100 m AGL",   "100agl",   100,  true },
  { "Vertical wind 200 m AGL",   "200agl",   200,  true },
  { "Vertical wind 400 m AGL",   "400agl",   400,  true },
  { "Vertical wind 800 m AGL",   "800agl",   800,  true },
};

[[gnu::pure]]
static bool
IsUKModel(unsigned model) noexcept
{
  return model == XCTHERM_MODEL_UK;
}

static const LayerInfo *
GetLayers(unsigned model, size_t &size) noexcept
{
  if (IsUKModel(model)) {
    size = std::size(UK_LAYERS);
    return UK_LAYERS;
  }
  size = std::size(CH_LAYERS);
  return CH_LAYERS;
}

[[gnu::pure]]
static bool
IsActiveLayer(const LayerInfo &layer,
              unsigned active_parameter,
              unsigned active_wave_height,
              unsigned active_vertical_agl) noexcept
{
  if (layer.is_agl)
    return active_parameter == 1 && layer.value == active_vertical_agl;
  else
    return active_parameter == 0 && layer.value == active_wave_height;
}

/**
 * Download metadata for a layer — shown in the second row.
 * For a span download, sizes/speed are totals across all hourly slices,
 * span_hours is the number of slices successfully fetched, and
 * pending_index/pending_total animate progress during the loop.
 */
struct LayerDownloadInfo {
  enum Status { NONE, PENDING, DONE, FAILED };
  Status status = NONE;
  double wire_mb = 0.0;           // total bytes over the network
  double size_mb = 0.0;           // total uncompressed size in memory
  double speed_mbs = 0.0;         // average download speed
  unsigned span_hours = 0;        // number of hourly slices downloaded
  unsigned pending_index = 0;     // current slice being fetched (1-based)
  unsigned pending_total = 0;     // total slices in this span
  std::string download_time;      // "HH:MM:SS" local time
};

/** Per-layer download info (indexed by layer position in the list) */
static LayerDownloadInfo download_info_ch[std::size(CH_LAYERS)];
static LayerDownloadInfo download_info_uk[std::size(UK_LAYERS)];

static LayerDownloadInfo *
GetDownloadInfo(unsigned model) noexcept
{
  return IsUKModel(model) ? download_info_uk : download_info_ch;
}

/* ---- List item renderer ---- */

class XCThermRowRenderer {
  TwoTextRowsRenderer row_renderer;

public:
  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font_bold,
                                       look.small_font);
  }

  void Draw(Canvas &canvas, const PixelRect rc, unsigned index,
            unsigned model, unsigned active_parameter,
            unsigned active_wave_height,
            unsigned active_vertical_agl);
};

void
XCThermRowRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          unsigned index, unsigned model,
                          unsigned active_parameter,
                          unsigned active_wave_height,
                          unsigned active_vertical_agl)
{
  size_t count = 0;
  const auto *layers = GetLayers(model, count);
  if (index >= count)
    return;

  const auto &layer = layers[index];
  const bool active = IsActiveLayer(layer, active_parameter,
                                     active_wave_height,
                                     active_vertical_agl);

  StaticString<80> first_row;
  if (active)
    first_row.Format("%s  [ACTIVE]", layer.label);
  else
    first_row = layer.label;

  /* Second row: download status */
  StaticString<200> second_row;
  const auto *info = GetDownloadInfo(model);
  switch (info[index].status) {
  case LayerDownloadInfo::PENDING:
    if (info[index].pending_total > 0)
      second_row.Format("Downloading %u/%uh...",
                        info[index].pending_index,
                        info[index].pending_total);
    else
      second_row = "Downloading...";
    break;
  case LayerDownloadInfo::DONE:
    second_row.Format("Heruntergeladen %uh: %.2f MB (%.2f MB wire, %.1f MB/s) | %s",
                      info[index].span_hours,
                      info[index].size_mb,
                      info[index].wire_mb,
                      info[index].speed_mbs,
                      info[index].download_time.c_str());
    break;
  case LayerDownloadInfo::FAILED:
    second_row = "Download fehlgeschlagen";
    break;
  default:
    if (active)
      second_row = "Nicht heruntergeladen";
    else
      second_row = "";
    break;
  }

  row_renderer.DrawFirstRow(canvas, rc, first_row);
  row_renderer.DrawSecondRow(canvas, rc, second_row);
}

/* ---- String choice renderer for model picker ---- */

class StringChoiceRenderer final : public ListItemRenderer {
  TextRowRenderer row_renderer;
  const char *const *choices;

public:
  explicit StringChoiceRenderer(const char *const *_choices) noexcept
    : choices(_choices) {}

  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override {
    row_renderer.DrawTextRow(canvas, rc, choices[i]);
  }
};

/* ---- Main widget ---- */

class XCThermWidget final : public ListWidget {
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *download_button = nullptr;
  Button *model_button = nullptr;
  Button *span_button = nullptr;

  XCThermRowRenderer row_renderer;

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

private:
  void UpdateList();
  void SaveSettings();

  void ActivateClicked();
  void DownloadClicked();
  void ModelClicked();
  void SpanClicked();

public:
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

protected:
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved(unsigned index) noexcept override;

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    ActivateClicked();
  }
};

void
XCThermWidget::CreateButtons(ButtonPanel &buttons)
{
  activate_button = buttons.Add("Activate", [this]() { ActivateClicked(); });
  download_button = buttons.Add("Download", [this]() { DownloadClicked(); });
  model_button = buttons.Add("Model", [this]() { ModelClicked(); });
  span_button = buttons.Add("Span", [this]() { SpanClicked(); });
}

void
XCThermWidget::SaveSettings()
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  Profile::Set(ProfileKeys::XCThermModel, (int)settings.model);
  Profile::Set(ProfileKeys::XCThermParameter, (int)settings.parameter);
  Profile::Set(ProfileKeys::XCThermWaveHeight, (int)settings.wave_height);
  Profile::Set(ProfileKeys::XCThermVerticalWindAGL,
               (int)settings.vertical_wind_agl);
  Profile::Set(ProfileKeys::XCThermDownloadSpan,
               (int)settings.download_span_hours);
}

void
XCThermWidget::UpdateList()
{
  ListControl &list = GetList();
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  list.SetLength(count);

  /* Find the currently active row */
  int active_index = -1;
  for (unsigned i = 0; i < count; ++i) {
    if (IsActiveLayer(layers[i], settings.parameter,
                       settings.wave_height,
                       settings.vertical_wind_agl)) {
      active_index = (int)i;
      break;
    }
  }

  if (active_index >= 0)
    list.SetCursorIndex(active_index);

  list.Invalidate();

  /* Update model button label */
  model_button->SetCaption(IsUKModel(settings.model)
                           ? "Model: UK" : "Model: CH");

  /* Update span button label */
  {
    StaticString<16> span_caption;
    span_caption.Format("Span: %uh", settings.download_span_hours);
    span_button->SetCaption(span_caption);
  }

  /* Update activate/update button state based on cursor */
  OnCursorMoved(list.GetCursorIndex());
}

void
XCThermWidget::OnCursorMoved(unsigned index) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const bool cursor_is_active = index < count &&
    IsActiveLayer(layers[index], settings.parameter,
                   settings.wave_height, settings.vertical_wind_agl);

  if (cursor_is_active) {
    activate_button->SetCaption("Active");
    activate_button->SetEnabled(false);
  } else {
    activate_button->SetCaption("Activate");
    activate_button->SetEnabled(true);
  }
}

void
XCThermWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  row_renderer.Draw(canvas, rc, idx,
                    settings.model, settings.parameter,
                    settings.wave_height, settings.vertical_wind_agl);
}

void
XCThermWidget::ActivateClicked()
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  const int index = GetList().GetCursorIndex();
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  if (index < 0 || (unsigned)index >= count)
    return;

  const auto &layer = layers[index];

  if (layer.is_agl) {
    settings.parameter = 1;
    settings.vertical_wind_agl = layer.value;
  } else {
    settings.parameter = 0;
    settings.wave_height = layer.value;
  }

  SaveSettings();
  UpdateList();
}

/* ---- Download span ---- */

void
XCThermWidget::DownloadClicked()
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  const unsigned span_hours = settings.download_span_hours;
  if (span_hours == 0)
    return;

  /* Download targets the cursor-selected row, not the active layer.
     This lets the user inspect/download any layer without first
     committing to it via Activate. */
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || (unsigned)cursor_index >= count) {
    ShowMessageBox("No layer selected.", "XCTherm", MB_OK);
    return;
  }
  const auto &target = layers[cursor_index];
  const int target_index = cursor_index;

  /* Build parameter name for API */
  StaticString<64> param;
  param.Format("vertical_wind_%s", target.file_suffix);

  /* Current UTC hour */
  unsigned current_utc = 12;
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    current_utc = basic.date_time_utc.hour;

  auto &api = XCThermAPI::Instance();
  api.SetCredentials(settings.credentials.email.c_str(),
                     settings.credentials.password.c_str());

  /* Ensure index is loaded */
  if (!api.IsIndexLoaded()) {
    if (!api.FetchIndex()) {
      ShowMessageBox("Failed to fetch forecast index.",
                     "XCTherm", MB_OK);
      return;
    }
  }

  auto *info = GetDownloadInfo(settings.model);
  auto &row_info = info[target_index];

  /* Reset and mark as pending */
  row_info = LayerDownloadInfo{};
  row_info.status = LayerDownloadInfo::PENDING;
  row_info.pending_total = span_hours;
  row_info.pending_index = 0;
  UpdateList();

  /* Loop over every hour from +1h to +span_hours, downloading and
     caching each. The map overlay is set to the first slice (closest
     to now) so the user gets immediate visual feedback. */
  XCThermGeoJSON::ForecastLayer first_forecast;
  bool any_failed = false;
  unsigned succeeded = 0;
  double total_wire_mb = 0.0;
  double total_disk_mb = 0.0;
  auto span_start = std::chrono::steady_clock::now();

  for (unsigned offset = 1; offset <= span_hours; ++offset) {
    row_info.pending_index = offset;
    UpdateList();

    std::string slot_date, slot_run_hour;
    unsigned slot_step;
    if (!api.FindSlotForOffset(param.c_str(), current_utc, offset,
                               slot_date, slot_run_hour, slot_step)) {
      LogFmt("xctherm: no slot for +{}h, skipping", offset);
      any_failed = true;
      continue;
    }

    std::string geojson;
    int64_t wire_bytes = 0;
    if (!api.DownloadGeoJSON(param.c_str(), slot_date,
                             slot_run_hour, slot_step, geojson,
                             nullptr, &wire_bytes)) {
      any_failed = true;
      continue;
    }

    total_wire_mb += (double)wire_bytes / (1024.0 * 1024.0);
    total_disk_mb += (double)geojson.size() / (1024.0 * 1024.0);
    ++succeeded;

    /* Parse the first slice that yields data so the map always
       gets refreshed (even if +1h is unavailable). */
    if (first_forecast.IsEmpty()) {
      auto forecast = XCThermGeoJSON::Parse(geojson, true);
      if (!forecast.IsEmpty()) {
        forecast.layer_name = target.label;
        first_forecast = std::move(forecast);
      }
    }
  }

  auto span_end = std::chrono::steady_clock::now();
  double span_secs = std::chrono::duration<double>(span_end - span_start).count();
  double speed_mbs = span_secs > 0 ? total_wire_mb / span_secs : 0.0;

  if (succeeded == 0) {
    row_info.status = LayerDownloadInfo::FAILED;
    UpdateList();
    return;
  }

  /* Replace the map overlay with this layer's forecast — regardless
     of which layer was previously active, the user-facing behaviour is
     "Download what I just selected, show what I just downloaded". */
  if (!first_forecast.IsEmpty()) {
    auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
    overlay->SetForecast(std::move(first_forecast), target.label);
    map->SetOverlay(std::move(overlay));
  }

  row_info.status = LayerDownloadInfo::DONE;
  row_info.size_mb = total_disk_mb;
  row_info.wire_mb = total_wire_mb;
  row_info.speed_mbs = speed_mbs;
  row_info.span_hours = succeeded;
  row_info.pending_index = 0;
  row_info.pending_total = 0;

  std::time_t now = std::time(nullptr);
  std::tm *lt = std::localtime(&now);
  char tbuf[16];
  std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt);
  row_info.download_time = tbuf;

  UpdateList();

  if (any_failed) {
    StaticString<128> msg;
    msg.Format("Downloaded %u of %u hourly slices.\n"
               "Some slots were unavailable.",
               succeeded, span_hours);
    ShowMessageBox(msg, "XCTherm", MB_OK);
  }
}

void
XCThermWidget::ModelClicked()
{
  static constexpr const char *choices[] = { "CH (Alps)", "UK" };

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  StringChoiceRenderer item_renderer(choices);

  int index = IsUKModel(settings.model) ? 1 : 0;
  index = ListPicker("Select model",
                     std::size(choices), index,
                     item_renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                     item_renderer,
                     false, nullptr, nullptr, nullptr);

  if (index < 0)
    return;

  settings.model = index == 1 ? XCTHERM_MODEL_UK : XCTHERM_MODEL_CH;
  SaveSettings();
  UpdateList();
}

void
XCThermWidget::SpanClicked()
{
  static constexpr const char *choices[] = {
    "1 hour", "3 hours", "6 hours", "12 hours", "18 hours", "24 hours",
  };
  static constexpr unsigned spans[] = { 1, 3, 6, 12, 18, 24 };

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  /* Pre-select the current value */
  int initial = 0;
  for (unsigned i = 0; i < std::size(spans); ++i) {
    if (spans[i] == settings.download_span_hours) {
      initial = (int)i;
      break;
    }
  }

  StringChoiceRenderer item_renderer(choices);
  int index = ListPicker("Download span",
                         std::size(choices), initial,
                         item_renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                         item_renderer,
                         false, nullptr, nullptr, nullptr);
  if (index < 0)
    return;

  settings.download_span_hours = spans[index];
  SaveSettings();
  UpdateList();
}

void
XCThermWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc) noexcept
{
  CreateButtons(buttons_widget->GetButtonPanel());
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, row_renderer.CalculateLayout(look));
  UpdateList();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermWidget() noexcept
{
  /*
   * If no XCTherm account is configured, show a simple message
   * (same pattern as pc_met).
   */
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  if (!settings.credentials.IsDefined())
    return std::make_unique<LargeTextWidget>(UIGlobals::GetDialogLook(),
                                             "No XCTherm account configured.\n\n"
                                             "Enter your credentials in\n"
                                             "Config > System > Weather.");

  auto widget = std::make_unique<XCThermWidget>();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(widget),
    ButtonPanelWidget::Alignment::BOTTOM);
  auto *widget_ptr = (XCThermWidget *)&buttons->GetWidget();
  widget_ptr->SetButtonPanel(*buttons);
  return buttons;
}

#endif
