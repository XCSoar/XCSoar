// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "PageActions.hpp"
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
#include "ui/event/PeriodicTimer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermDownloadGlue.hpp"
#include "Weather/xctherm/XCThermDownloadJob.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "LogFile.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "net/http/Init.hpp"

#include <chrono>
#include <ctime>
#include <memory>
#include <optional>

namespace {

/* Model IDs — keep in sync with WeatherConfigPanel's region enum. */
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
  enum Status { NONE, PENDING, DONE, FAILED, CANCELED };
  Status status = NONE;
  double wire_mb = 0.0;           // total bytes over the network
  double speed_mbs = 0.0;         // average download speed
  unsigned span_hours = 0;        // hours of forecast cached (incl. reuse)
  unsigned future_hours = 0;      // cached hours still in the future from now
  unsigned new_downloads = 0;     // slices newly fetched this run
  unsigned pending_index = 0;     // current slice being fetched (1-based)
  unsigned pending_total = 0;     // total slices in this span
  uint64_t pending_bytes_now = 0; // bytes received on current slice
  uint64_t pending_bytes_total = 0; // expected total of current slice
  unsigned retry_attempt = 0;     // 0 = not retrying; >0 = nth retry
  unsigned retry_seconds_left = 0;// seconds until next retry attempt
  std::string download_time;      // "HH:MM:SS" local time
  std::string issued_utc;         // "YYYY-MM-DD HH UTC" of the model run
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
            unsigned active_vertical_agl,
            unsigned span_hours_setting);
};

void
XCThermRowRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          unsigned index, unsigned model,
                          unsigned active_parameter,
                          unsigned active_wave_height,
                          unsigned active_vertical_agl,
                          unsigned span_hours_setting)
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
    first_row.Format("%s  %s", layer.label, _("[ACTIVE]"));
  else
    first_row = layer.label;

  /* Second row: download status */
  StaticString<200> second_row;
  const auto *info = GetDownloadInfo(model);
  switch (info[index].status) {
  case LayerDownloadInfo::PENDING: {
    const auto &p = info[index];
    /* pending_index is the 1-based slot number being fetched (1 = the
       previous-hour slice we always prepend, 2..pending_total = future
       hours). "Slot N/M" rather than "+Nh of M" because the offset for
       N=1 is actually -1h. */
    if (p.retry_seconds_left > 0) {
      /* Network seems down — show countdown to next attempt. */
      second_row.Format(_("Slot %u: reconnect in %us (try #%u)"),
                        p.pending_index,
                        p.retry_seconds_left,
                        p.retry_attempt + 1);
    } else if (p.pending_bytes_total > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      const double tot_mb = (double)p.pending_bytes_total / (1024.0 * 1024.0);
      second_row.Format(_("Slot %u/%u: %.2f / %.2f MB"),
                        p.pending_index,
                        p.pending_total,
                        now_mb, tot_mb);
    } else if (p.pending_bytes_now > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      second_row.Format(_("Slot %u/%u: %.2f MB"),
                        p.pending_index,
                        p.pending_total,
                        now_mb);
    } else if (p.pending_total > 0) {
      second_row.Format(_("Slot %u/%u: connecting..."),
                        p.pending_index,
                        p.pending_total);
    } else {
      second_row = _("Connecting...");
    }
    break;
  }
  case LayerDownloadInfo::DONE: {
    /* "X/Yh" where X = cached future-hours still ahead of now and
       Y = the user's currently configured download span. Letting the
       user see at-a-glance how much of a fresh fetch they'd actually
       need: e.g. "4 / 12 h" means the layer covers 4 of the next 12
       requested hours; the rest would be new downloads. */
    const unsigned future = info[index].future_hours;
    if (info[index].new_downloads == 0)
      second_row.Format(_("%u/%uh | Issued %s | %s"),
                        future, span_hours_setting,
                        info[index].issued_utc.c_str(),
                        info[index].download_time.c_str());
    else
      second_row.Format(_("%u/%uh (%u new) | Issued %s | %.2f MB wire %.1f MB/s | %s"),
                        future, span_hours_setting,
                        info[index].new_downloads,
                        info[index].issued_utc.c_str(),
                        info[index].wire_mb,
                        info[index].speed_mbs,
                        info[index].download_time.c_str());
    break;
  }
  case LayerDownloadInfo::FAILED:
    second_row = _("Download failed");
    break;
  case LayerDownloadInfo::CANCELED:
    second_row = _("Cancelled");
    break;
  default:
    if (active)
      second_row = _("Not downloaded");
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
    /* Choices are stored with N_() markers and translated at paint
       time so callers can pass plain string literals into the array
       and still get localized display. */
    row_renderer.DrawTextRow(canvas, rc, gettext(choices[i]));
  }
};

/* ---- Main widget ---- */

class XCThermWidget final : public ListWidget {
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *span_button = nullptr;
  Button *activate_button = nullptr;
  Button *download_button = nullptr;   // labeled "Update" or "Stop"
  Button *delete_button = nullptr;

  XCThermRowRenderer row_renderer;

  std::shared_ptr<XCThermDownloadJob> active_job;

  std::optional<XCThermDownloadGlue> download_glue;

  /* Polls active_job every 200 ms for live progress (UI thread). */
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

  ~XCThermWidget() noexcept override {
    if (download_glue)
      download_glue->RequestCancel();
    if (download_glue)
      download_glue->BeginShutdown();
  }

private:
  void UpdateList();
  void SaveSettings();

  void ActivateClicked();
  void DownloadClicked();
  void DeleteClicked();
  void SpanClicked();

  /* New download flow */
  void StartDownload();
  void PollDownload();
  void FinishDownload();
  void CancelDownload();

  /**
   * Walk every row for the current model and populate its
   * LayerDownloadInfo from the XCThermAPI in-RAM cache (which on
   * startup includes anything the persistent disk cache rehydrated).
   * Lets the user open the dialog and immediately see that prior-
   * session forecasts are still available, with their freshness and
   * the run they came from, without having to redownload.
   *
   * Never stomps on session state (PENDING / FAILED / CANCELED entries
   * are left alone), so this is safe to call from Prepare().
   */
  void RehydrateRowsFromCache() noexcept;

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
  /* Button order matches user-flow:
       1. Span  — pick how much to fetch
       2. Activate — mark which row is "active"
       3. Update — fetch / refresh data (acts as Stop while running)
       4. Delete — clear cached data for the cursor row
     Region (CH/UK) lives in Config → System → Weather. */
  span_button = buttons.Add(_("Span"), [this]() { SpanClicked(); });
  activate_button = buttons.Add(_("Activate"),
                                [this]() { ActivateClicked(); });
  download_button = buttons.Add(_("Update"),
                                [this]() { DownloadClicked(); });
  delete_button = buttons.Add(_("Delete"),
                              [this]() { DeleteClicked(); });
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
  /* download_span_hours is session-only — not persisted. */
}

void
XCThermWidget::UpdateList()
{
  ListControl &list = GetList();
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  GetLayers(settings.model, count);

  list.SetLength(count);

  /* Important: do NOT call SetCursorIndex here. UpdateList runs after
     every action (download progress tick, Activate, Span change). If
     we re-positioned the cursor to the active row each time, a user
     who downloaded a non-active layer would see the cursor jump back
     to the active row on completion — losing track of what they just
     fetched. Initial cursor positioning happens once in Prepare(). */

  list.Invalidate();

  /* Update span button label */
  {
    StaticString<16> span_caption;
    span_caption.Format("Span: %uh", settings.download_span_hours);
    span_button->SetCaption(span_caption);
  }

  /* Flip Update button to Stop while a job is running; freeze the
     other settings buttons so the job's target/span can't change
     underneath the worker. */
  const bool job_running = (bool)active_job;
  download_button->SetCaption(job_running ? _("Stop") : _("Update"));
  if (span_button)   span_button->SetEnabled(!job_running);
  if (delete_button) delete_button->SetEnabled(!job_running);

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
    activate_button->SetCaption(_("Active"));
    activate_button->SetEnabled(false);
  } else {
    activate_button->SetCaption(_("Activate"));
    activate_button->SetEnabled(true);
  }

  /* While a download is running, freeze Activate so we don't change
     the active layer concept mid-flight. */
  if (active_job)
    activate_button->SetEnabled(false);
}

void
XCThermWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  row_renderer.Draw(canvas, rc, idx,
                    settings.model, settings.parameter,
                    settings.wave_height, settings.vertical_wind_agl,
                    settings.download_span_hours);
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

/* ---- Delete cached data for the cursor-selected layer ---- */

void
XCThermWidget::DeleteClicked()
{
  /* Don't allow Delete while a download is running — would race with
     the worker writing fresh entries into the cache. */
  if (active_job)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || (unsigned)cursor_index >= count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }
  const auto &target = layers[cursor_index];

  StaticString<64> param;
  param.Format("vertical_wind_%s", target.file_suffix);

  XCThermAPI::Instance().ClearLayer(param.c_str());

  /* Reset the per-row status so the dialog stops showing the old
     "Cached Nh | Issued …" line. */
  auto *info = GetDownloadInfo(settings.model);
  info[cursor_index] = LayerDownloadInfo{};

  /* If the deleted layer was currently displayed on the map, clear the
     overlay. We don't try to detect that exactly — easier to just drop
     it whenever the user invokes Delete; auto-switch will repopulate
     from a different cached layer on the next GPS tick if available. */
  if (auto *map = UIGlobals::GetMap())
    map->SetOverlay(nullptr);

  UpdateList();
}

/* ---- Download span (asynchronous worker thread) ---- */

void
XCThermWidget::DownloadClicked()
{
  /* The download button doubles as Stop. */
  if (active_job) {
    CancelDownload();
    return;
  }
  StartDownload();
}

void
XCThermWidget::CancelDownload()
{
  if (!active_job)
    return;

  if (download_glue)
    download_glue->RequestCancel();
  else
    active_job->cancel.store(true);
}

void
XCThermWidget::StartDownload()
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  const unsigned span_hours = settings.download_span_hours;
  if (span_hours == 0)
    return;

  /* Download targets the cursor-selected row, not the active layer. */
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || (unsigned)cursor_index >= count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }
  const auto &target = layers[cursor_index];

  /* Current UTC hour — captured up-front because the worker can't safely
     touch CommonInterface (UI-only). */
  unsigned current_utc = 12;
  {
    const auto &basic = CommonInterface::Basic();
    if (basic.date_time_utc.IsPlausible())
      current_utc = basic.date_time_utc.hour;
  }

  auto &api = XCThermAPI::Instance();
  /* Idempotent disk cache bootstrap — guarantees the in-RAM cache is
     populated from previous sessions before we start the download
     loop (so IsCachedAtRun can skip slices we already have on disk). */
  api.EnableDiskCache();
  api.SetCredentials(settings.credentials.email.c_str(),
                     settings.credentials.password.c_str());

  /* Index fetch happens synchronously on the UI thread once, before
     spawning the worker — it's a small payload and the worker can
     then assume index_loaded == true. FetchIndex throws on real
     failures; ShowError surfaces the actual cause to the user (auth
     vs network vs server error, etc.). */
  if (!api.IsIndexLoaded()) {
    try {
      if (!api.FetchIndex()) {
        ShowMessageBox(_("Forecast index has no XCTherm parameters."),
                       "XCTherm", MB_OK);
        return;
      }
    } catch (...) {
      ShowError(std::current_exception(), "XCTherm");
      return;
    }
  }

  if (Net::curl == nullptr) {
    ShowMessageBox(_("Network is not available."), "XCTherm", MB_OK);
    return;
  }

  if (!download_glue)
    download_glue.emplace(*Net::curl);

  if (download_glue->IsRunning())
    return;

  auto job = std::make_shared<XCThermDownloadJob>();
  job->model = settings.model;
  job->target_index = cursor_index;
  job->target_label = target.label;
  job->param = std::string("vertical_wind_") + target.file_suffix;
  job->span_hours = span_hours;
  job->current_utc = current_utc;
  job->started_at = std::chrono::steady_clock::now();

  auto *info = GetDownloadInfo(settings.model);
  auto &row_info = info[cursor_index];
  row_info.status = LayerDownloadInfo::PENDING;
  row_info.pending_total = span_hours + 1;
  row_info.pending_index = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

  active_job = job;

  download_glue->Start(job, [this](std::shared_ptr<XCThermDownloadJob> finished) {
    active_job = std::move(finished);
    FinishDownload();
  });

  UpdateList();
  poll_timer.Schedule(std::chrono::milliseconds(200));
}

void
XCThermWidget::PollDownload()
{
  if (!active_job)
    return;

  auto &job = *active_job;

  /* Mirror live atomics into the row state so the renderer can pick
     them up on the next paint. */
  auto *info = GetDownloadInfo(job.model);
  auto &row_info = info[job.target_index];
  row_info.pending_index = job.current_offset.load();
  row_info.pending_total = job.span_hours + 1;
  row_info.pending_bytes_now = job.bytes_now.load();
  row_info.pending_bytes_total = job.bytes_total.load();
  row_info.retry_attempt = job.retry_attempt.load();
  row_info.retry_seconds_left = job.retry_seconds_left.load();

  GetList().Invalidate();
}

void
XCThermWidget::FinishDownload()
{
  if (!active_job)
    return;

  poll_timer.Cancel();

  auto job = std::move(active_job);
  active_job.reset();

  auto *map = UIGlobals::GetMap();
  auto *info = GetDownloadInfo(job->model);
  auto &row_info = info[job->target_index];

  const unsigned span = job->span_hours;
  const unsigned ok    = job->succeeded_or_cached.load();
  const unsigned nu    = job->newly_downloaded.load();
  const bool canceled  = job->cancel.load();
  const bool any_miss  = job->any_slot_missing.load();

  /* Decide the post-run row state. */
  if (ok == 0) {
    row_info.status = canceled
      ? LayerDownloadInfo::CANCELED
      : LayerDownloadInfo::FAILED;
    row_info.pending_index = 0;
    row_info.pending_total = 0;
    row_info.pending_bytes_now = 0;
    row_info.pending_bytes_total = 0;
    row_info.retry_attempt = 0;
    row_info.retry_seconds_left = 0;
    UpdateList();
    if (canceled) {
      /* nothing — Cancelled state in the row says it all */
    } else if (job->error_eptr) {
      /* Surface the actual cause (auth, forbidden, server error, …)
         instead of a generic message. ShowError unpacks the chain of
         what() strings from the exception_ptr. */
      ShowError(job->error_eptr, "XCTherm");
    } else {
      ShowMessageBox(_("Forecast download failed.\nKeeping previous data."),
                     "XCTherm", MB_OK);
    }
    return;
  }

  /* Atomic overlay swap: only after the worker has produced a parseable
     slice (and only if any actual transfer happened — pure cache hits
     reuse the existing overlay if there is one). */
  std::lock_guard lock{job->result_mutex};
  if (map != nullptr && !job->first_forecast.IsEmpty()) {
    /* The first parseable slice the worker kept is the previous-hour
       slot (slot 0), valid at current_utc - 1. */
    const unsigned shown_utc = (job->current_utc + 23) % 24;
    auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
    overlay->SetForecast(std::move(job->first_forecast),
                         job->target_label.c_str(),
                         job->param.c_str(), shown_utc);
    map->SetOverlay(std::move(overlay));
  }

  const double span_secs = std::chrono::duration<double>(
    std::chrono::steady_clock::now() - job->started_at).count();
  const double wire_mb = (double)job->total_wire_bytes.load() / (1024.0 * 1024.0);
  const double speed_mbs = span_secs > 0 ? wire_mb / span_secs : 0.0;

  row_info.status = LayerDownloadInfo::DONE;
  row_info.wire_mb = wire_mb;
  row_info.speed_mbs = speed_mbs;
  row_info.span_hours = ok;
  row_info.new_downloads = nu;
  /* Recompute the count of future-valid slices so the "X/Yh" cell
     reflects this fresh download immediately. */
  row_info.future_hours = XCThermAPI::Instance()
    .GetCachedLayerSummary(job->param).future_hours;
  row_info.pending_index = 0;
  row_info.pending_total = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

  if (job->latest_run_date.size() == 8 && job->latest_run_hour.size() == 2) {
    const std::string &d = job->latest_run_date;
    row_info.issued_utc = std::string(FmtBuffer<32>("{}-{}-{} {} UTC",
                                                    d.substr(0, 4),
                                                    d.substr(4, 2),
                                                    d.substr(6, 2),
                                                    job->latest_run_hour).c_str());
  } else {
    row_info.issued_utc = "?";
  }

  std::time_t now = std::time(nullptr);
  std::tm *lt = std::localtime(&now);
  char tbuf[16];
  std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt);
  row_info.download_time = tbuf;

  /* Stale-run sweep — but only AFTER the new forecast is successfully
     in the cache and the overlay was swapped above. Older-run entries
     for hours we didn't re-fetch this round get dropped here so the
     cache for this layer is consistent (one run per layer). Past
     hours stay in the cache (user can browse back); we just align
     them with the latest run. */
  if (nu > 0) {
    unsigned current_utc = job->current_utc;
    const auto &basic = CommonInterface::Basic();
    if (basic.date_time_utc.IsPlausible())
      current_utc = basic.date_time_utc.hour;

    const unsigned dropped =
      XCThermAPI::Instance().PruneStaleRuns(job->param, current_utc);
    if (dropped > 0)
      LogFmt("xctherm: stale-run sweep dropped {} entries for {}",
             dropped, job->param);
  }

  UpdateList();

  /* Trigger a page reload so PageActions::LoadBottom re-evaluates
     XCThermAPI::HasAnyCache() and the cursor bar appears on the
     current XCTherm page (instead of waiting for the next manual
     page switch). Cheap on success — does nothing if the current page
     isn't configured for XCTherm. */
  if (nu > 0)
    PageActions::Update();

  if (job->error_eptr && !canceled) {
    /* Partial success: some slots got through, then a persistent
       error stopped us. Surface the cause via ShowError so the user
       knows what to fix. */
    ShowError(job->error_eptr, "XCTherm");
  } else if (any_miss && !canceled) {
    StaticString<128> msg;
    msg.Format(_("Got %u of %u hourly slices (%u newly downloaded).\n"
                 "Some slots were unavailable."),
               ok, span, nu);
    ShowMessageBox(msg, "XCTherm", MB_OK);
  }
}

void
XCThermWidget::SpanClicked()
{
  /* These strings are localized via gettext() at render time inside
     StringChoiceRenderer; mark them with N_() so they're picked up
     into the translation catalog. */
  static constexpr const char *choices[] = {
    N_("1 hour"), N_("3 hours"), N_("6 hours"),
    N_("12 hours"), N_("18 hours"),
  };
  static constexpr unsigned spans[] = { 1, 3, 6, 12, 18 };
  static_assert(std::size(choices) == std::size(spans));

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
  int index = ListPicker(_("Download span"),
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
XCThermWidget::RehydrateRowsFromCache() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);
  auto *info = GetDownloadInfo(settings.model);
  auto &api = XCThermAPI::Instance();

  /* Make sure the disk index is built before we read it — otherwise a
     fresh session that opens this dialog without having downloaded
     anything yet would show "Not downloaded" for slices that are in
     fact sitting in the on-disk cache. Idempotent. */
  api.EnableDiskCache();

  for (unsigned i = 0; i < count; ++i) {
    /* Don't overwrite session state — a row that's mid-download
       (PENDING) or failed/canceled should keep its visible status. */
    if (info[i].status != LayerDownloadInfo::NONE)
      continue;

    StaticString<64> param;
    param.Format("vertical_wind_%s", layers[i].file_suffix);

    const auto summary = api.GetCachedLayerSummary(param.c_str());
    if (summary.hours.empty())
      continue;

    LayerDownloadInfo &row = info[i];
    row.status = LayerDownloadInfo::DONE;
    row.span_hours = (unsigned)summary.hours.size();
    row.future_hours = summary.future_hours;
    row.new_downloads = 0;
    row.wire_mb = 0.0;
    row.speed_mbs = 0.0;
    row.pending_index = 0;
    row.pending_total = 0;
    row.pending_bytes_now = 0;
    row.pending_bytes_total = 0;
    row.retry_attempt = 0;
    row.retry_seconds_left = 0;

    if (summary.latest_run_date.size() == 8 &&
        summary.latest_run_hour.size() == 2) {
      const std::string &d = summary.latest_run_date;
      row.issued_utc = std::string(FmtBuffer<32>("{}-{}-{} {} UTC",
                                                 d.substr(0, 4),
                                                 d.substr(4, 2),
                                                 d.substr(6, 2),
                                                 summary.latest_run_hour).c_str());
    } else {
      row.issued_utc = "?";
    }

    if (summary.latest_downloaded_at > 0) {
      const std::time_t t = (std::time_t)summary.latest_downloaded_at;
      std::tm *lt = std::localtime(&t);
      char tbuf[16];
      if (lt && std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt) > 0)
        row.download_time = tbuf;
    }
  }
}

void
XCThermWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc) noexcept
{
  if (Net::curl != nullptr && !download_glue)
    download_glue.emplace(*Net::curl);

  CreateButtons(buttons_widget->GetButtonPanel());
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, row_renderer.CalculateLayout(look));

  /* Pull any forecasts that the persistent disk cache restored at app
     startup into the per-row LayerDownloadInfo, so the user sees them
     as cached (with freshness + run info) the first time they open the
     dialog this session — not as "Not downloaded". */
  RehydrateRowsFromCache();

  UpdateList();

  /* Seed the cursor to the currently active layer — but only here,
     once, on initial display. UpdateList() deliberately does NOT touch
     the cursor on subsequent calls so a user who downloads layer X
     while layer Y is active stays parked on X after the download. */
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);
  for (unsigned i = 0; i < count; ++i) {
    if (IsActiveLayer(layers[i], settings.parameter,
                      settings.wave_height,
                      settings.vertical_wind_agl)) {
      GetList().SetCursorIndex(i);
      break;
    }
  }
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
                                             _("No XCTherm account configured.\n\n"
                                               "Enter your credentials in\n"
                                               "Config > System > Weather."));

  auto widget = std::make_unique<XCThermWidget>();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(widget),
    ButtonPanelWidget::Alignment::BOTTOM);
  auto *widget_ptr = static_cast<XCThermWidget *>(&buttons->GetWidget());
  widget_ptr->SetButtonPanel(*buttons);
  return buttons;
}

#endif
