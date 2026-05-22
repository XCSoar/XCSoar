// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Error.hpp"
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
#include "ui/event/PeriodicTimer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "LogFile.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

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
  double size_mb = 0.0;           // total uncompressed size in memory
  double speed_mbs = 0.0;         // average download speed
  unsigned span_hours = 0;        // hours of forecast cached (incl. reuse)
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

/**
 * Cross-thread state for a span download.
 *
 * Owned via std::shared_ptr so that the UI widget can drop its reference
 * mid-flight (when the dialog closes) while the worker thread keeps the
 * job alive long enough to observe the cancel flag and exit cleanly.
 *
 * Atomics carry live progress that the UI thread polls every 200 ms via
 * PeriodicTimer. The result_mutex-guarded fields hold the final outputs
 * the UI applies once @c done is observed true.
 */
struct DownloadJob {
  /* Inputs (read-only after construction) */
  unsigned model = 0;
  int target_index = -1;
  std::string target_label;
  std::string param;
  unsigned span_hours = 0;
  unsigned current_utc = 12;

  /* Live progress, written by worker, read by UI */
  std::atomic<bool> cancel{false};
  std::atomic<bool> done{false};
  std::atomic<unsigned> current_offset{0};
  std::atomic<uint64_t> bytes_now{0};
  std::atomic<uint64_t> bytes_total{0};
  std::atomic<uint64_t> total_wire_bytes{0};
  std::atomic<uint64_t> total_disk_bytes{0};
  std::atomic<unsigned> succeeded_or_cached{0};
  std::atomic<unsigned> newly_downloaded{0};
  std::atomic<unsigned> retry_attempt{0};
  std::atomic<unsigned> retry_seconds_left{0};
  std::atomic<bool> any_slot_missing{false};

  /* Final results, written once by worker before setting done=true */
  std::mutex result_mutex;
  std::exception_ptr error;
  XCThermGeoJSON::ForecastLayer first_forecast;
  std::string latest_run_date;
  std::string latest_run_hour;
  std::chrono::steady_clock::time_point started_at;
  std::chrono::steady_clock::time_point finished_at;
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
  case LayerDownloadInfo::PENDING: {
    const auto &p = info[index];
    if (p.retry_seconds_left > 0) {
      /* Network seems down — show countdown to next attempt. */
      second_row.Format("+%uh: reconnect in %us (try #%u)",
                        p.pending_index,
                        p.retry_seconds_left,
                        p.retry_attempt + 1);
    } else if (p.pending_bytes_total > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      const double tot_mb = (double)p.pending_bytes_total / (1024.0 * 1024.0);
      second_row.Format("+%uh of %uh: %.2f / %.2f MB",
                        p.pending_index,
                        p.pending_total,
                        now_mb, tot_mb);
    } else if (p.pending_bytes_now > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      second_row.Format("+%uh of %uh: %.2f MB",
                        p.pending_index,
                        p.pending_total,
                        now_mb);
    } else if (p.pending_total > 0) {
      second_row.Format("+%uh of %uh: connecting...",
                        p.pending_index,
                        p.pending_total);
    } else {
      second_row = "Connecting...";
    }
    break;
  }
  case LayerDownloadInfo::DONE:
    /* Show issued time so the user can tell whether a layer is from the
       latest run; new_downloads vs span_hours shows how much of this
       click was reused from cache vs newly fetched. */
    if (info[index].new_downloads == 0)
      second_row.Format("Cached %uh | Issued %s | %s",
                        info[index].span_hours,
                        info[index].issued_utc.c_str(),
                        info[index].download_time.c_str());
    else
      second_row.Format("%uh (%u new) | Issued %s | %.2f MB wire %.1f MB/s | %s",
                        info[index].span_hours,
                        info[index].new_downloads,
                        info[index].issued_utc.c_str(),
                        info[index].wire_mb,
                        info[index].speed_mbs,
                        info[index].download_time.c_str());
    break;
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
    row_renderer.DrawTextRow(canvas, rc, choices[i]);
  }
};

/* ---- Main widget ---- */

class XCThermWidget final : public ListWidget {
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *download_button = nullptr;
  Button *span_button = nullptr;

  XCThermRowRenderer row_renderer;

  /* In-flight download. nullptr when idle. The widget keeps a strong
     reference; the worker thread captures its own shared_ptr so the
     job object survives even if the widget is destroyed mid-flight. */
  std::shared_ptr<DownloadJob> active_job;

  /* Polls active_job every 200 ms to refresh the list row and detect
     completion. Fires on the UI thread. */
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

  ~XCThermWidget() noexcept override {
    /* If a download is still running, ask it to stop and detach: the
       worker keeps a shared_ptr to the job, so cancel will be observed
       on the next progress callback or retry-sleep tick. */
    if (active_job) {
      active_job->cancel.store(true);
      if (active_job_thread.joinable())
        active_job_thread.detach();
    }
  }

private:
  std::thread active_job_thread;

  void UpdateList();
  void SaveSettings();

  void ActivateClicked();
  void DownloadClicked();
  void SpanClicked();

  /* New download flow */
  void StartDownload();
  void PollDownload();
  void FinishDownload();
  void CancelDownload();
  static void DownloadWorker(std::shared_ptr<DownloadJob> job);

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
  /* This button flips between "Download" (idle) and "Stop" (running)
     in UpdateList(); the click handler dispatches by current state. */
  download_button = buttons.Add("Download", [this]() { DownloadClicked(); });
  span_button = buttons.Add("Span", [this]() { SpanClicked(); });
  /* Region (CH/UK) is now exposed in Config → System → Weather, like
     OpenSoar handles Skysight region — see WeatherConfigPanel. */
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
  GetLayers((unsigned)settings.model, count);

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

  /* Flip Download button to Stop while a job is running; freeze the
     other settings buttons so the job's target/span can't change
     underneath the worker. */
  const bool job_running = (bool)active_job;
  download_button->SetCaption(job_running ? "Stop" : "Download");
  if (span_button)  span_button->SetEnabled(!job_running);

  /* Update activate/update button state based on cursor */
  OnCursorMoved(list.GetCursorIndex());
}

void
XCThermWidget::OnCursorMoved(unsigned index) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers((unsigned)settings.model, count);

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
                    (unsigned)settings.model, settings.parameter,
                    settings.wave_height, settings.vertical_wind_agl);
}

void
XCThermWidget::ActivateClicked()
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  const int index = GetList().GetCursorIndex();
  size_t count = 0;
  const auto *layers = GetLayers((unsigned)settings.model, count);

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
  /* Setting the cancel flag aborts the in-flight curl call (via the
     progress callback returning non-zero) and shortcuts the
     retry-sleep loop. The worker then sets done=true; the poll timer
     picks that up and runs FinishDownload(). */
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
  const auto *layers = GetLayers((unsigned)settings.model, count);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || (unsigned)cursor_index >= count) {
    ShowMessageBox(_("No layer selected."), _("XCTherm"), MB_OK);
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

  try {
    api.SetCredentials(settings.credentials.email.c_str(),
                       settings.credentials.password.c_str());

    /* Index fetch happens synchronously on the UI thread once, before
       spawning the worker — it's a small payload and the worker can
       then assume index_loaded == true. */
    if (!api.IsIndexLoaded() && !api.FetchIndex())
      throw std::runtime_error("Failed to fetch forecast index");

    /* Initialise per-row UI state and the job descriptor. */
    auto job = std::make_shared<DownloadJob>();
    job->model = (unsigned)settings.model;
    job->target_index = cursor_index;
    job->target_label = target.label;
    job->param = std::string("vertical_wind_") + target.file_suffix;
    job->span_hours = span_hours;
    job->current_utc = current_utc;
    job->started_at = std::chrono::steady_clock::now();

    auto *info = GetDownloadInfo((unsigned)settings.model);
    auto &row_info = info[cursor_index];
    /* Preserve the previous DONE info so FinishDownload can restore it
       if nothing usable comes out of the new run. The PENDING row text
       is rendered from atomics, not from these fields. */
    row_info.status = LayerDownloadInfo::PENDING;
    row_info.pending_total = span_hours;
    row_info.pending_index = 0;
    row_info.pending_bytes_now = 0;
    row_info.pending_bytes_total = 0;
    row_info.retry_attempt = 0;
    row_info.retry_seconds_left = 0;

    active_job = job;
    /* Spawn the worker. It captures its own shared_ptr to the job so the
       state survives even if the widget is destroyed mid-flight. */
    active_job_thread = std::thread(&XCThermWidget::DownloadWorker, job);
  } catch (...) {
    ShowError(std::current_exception(), "XCTherm");
    return;
  }

  /* Refresh button labels (Download → Stop) and start the UI poll. */
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
  row_info.pending_total = job.span_hours;
  row_info.pending_bytes_now = job.bytes_now.load();
  row_info.pending_bytes_total = job.bytes_total.load();
  row_info.retry_attempt = job.retry_attempt.load();
  row_info.retry_seconds_left = job.retry_seconds_left.load();

  GetList().Invalidate();

  if (job.done.load())
    FinishDownload();
}

void
XCThermWidget::FinishDownload()
{
  if (!active_job)
    return;

  poll_timer.Cancel();

  /* The worker has signalled done; safe to join. */
  if (active_job_thread.joinable())
    active_job_thread.join();

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
    if (!canceled) {
      if (job->error)
        ShowError(job->error, "XCTherm");
      else
        ShowError(std::make_exception_ptr(
                    std::runtime_error("Forecast download failed")),
                  "XCTherm");
    }
    return;
  }

  /* Atomic overlay swap: only after the worker has produced a parseable
     slice (and only if any actual transfer happened — pure cache hits
     reuse the existing overlay if there is one). */
  try {
    std::lock_guard lock{job->result_mutex};
    if (map != nullptr && !job->first_forecast.IsEmpty()) {
      auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
      overlay->SetForecast(std::move(job->first_forecast),
                           job->target_label.c_str());
      map->SetOverlay(std::move(overlay));
    }
  } catch (...) {
    ShowError(std::current_exception(), "XCTherm");
  }

  const double span_secs = std::chrono::duration<double>(
    std::chrono::steady_clock::now() - job->started_at).count();
  const double wire_mb = (double)job->total_wire_bytes.load() / (1024.0 * 1024.0);
  const double disk_mb = (double)job->total_disk_bytes.load() / (1024.0 * 1024.0);
  const double speed_mbs = span_secs > 0 ? wire_mb / span_secs : 0.0;

  row_info.status = LayerDownloadInfo::DONE;
  row_info.size_mb = disk_mb;
  row_info.wire_mb = wire_mb;
  row_info.speed_mbs = speed_mbs;
  row_info.span_hours = ok;
  row_info.new_downloads = nu;
  row_info.pending_index = 0;
  row_info.pending_total = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

  if (job->latest_run_date.size() == 8 && job->latest_run_hour.size() == 2) {
    char issued[24];
    std::snprintf(issued, sizeof(issued), "%.4s-%.2s-%.2s %s UTC",
                  job->latest_run_date.c_str(),
                  job->latest_run_date.c_str() + 4,
                  job->latest_run_date.c_str() + 6,
                  job->latest_run_hour.c_str());
    row_info.issued_utc = issued;
  } else {
    row_info.issued_utc = "?";
  }

  std::time_t now = std::time(nullptr);
  std::tm *lt = std::localtime(&now);
  char tbuf[16];
  std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt);
  row_info.download_time = tbuf;

  UpdateList();

  if (any_miss && !canceled) {
    StaticString<128> msg;
    msg.Format("Got %u of %u hourly slices (%u newly downloaded).\n"
               "Some slots were unavailable.",
               ok, span, nu);
    ShowMessageBox(msg, _("XCTherm"), MB_OK);
  }
}

/* ---- Download worker (background thread) ---- */

void
XCThermWidget::DownloadWorker(std::shared_ptr<DownloadJob> job)
{
  try {
    auto &api = XCThermAPI::Instance();

    for (unsigned offset = 1; offset <= job->span_hours; ++offset) {
    if (job->cancel.load())
      break;

    job->current_offset.store(offset);
    job->bytes_now.store(0);
    job->bytes_total.store(0);

    std::string slot_date, slot_run_hour;
    unsigned slot_step = 0;
    if (!api.FindSlotForOffset(job->param, job->current_utc, offset,
                               slot_date, slot_run_hour, slot_step)) {
      LogFmt("xctherm: no slot for +{}h — skipping", offset);
      job->any_slot_missing.store(true);
      continue;
    }

    const unsigned run_h = (unsigned)std::atoi(slot_run_hour.c_str());
    const unsigned forecast_utc = (run_h + slot_step) % 24;

    /* Track the freshest run seen for the display. */
    {
      std::lock_guard lock{job->result_mutex};
      if (slot_date > job->latest_run_date ||
          (slot_date == job->latest_run_date &&
           slot_run_hour > job->latest_run_hour)) {
        job->latest_run_date = slot_date;
        job->latest_run_hour = slot_run_hour;
      }
    }

    /* Skip slices that are already cached from this exact run. */
    if (api.IsCachedAtRun(job->param, forecast_utc,
                          slot_date, slot_run_hour)) {
      LogFmt("xctherm: +{}h ({}UTC) cached at run {}/{}Z — reused",
             offset, forecast_utc, slot_date, slot_run_hour);
      job->succeeded_or_cached.fetch_add(1);

      /* Use the cached slice for the overlay if this is the first
         parseable one we've seen. */
      {
        std::lock_guard lock{job->result_mutex};
        if (job->first_forecast.IsEmpty()) {
          const std::string cached =
            api.GetCachedGeoJSON(job->param, forecast_utc);
          if (!cached.empty()) {
            auto forecast = XCThermGeoJSON::Parse(cached, true);
            if (!forecast.IsEmpty()) {
              forecast.layer_name = job->target_label;
              job->first_forecast = std::move(forecast);
            }
          }
        }
      }
      continue;
    }

    /* Retry loop for this slot. Each iteration: one curl call. On
       failure we sleep with cancel checks and try again — forever
       until the user cancels or the network comes back. This matches
       the "in flight, weak Wi-Fi, just keep trying" requirement. */
    bool slot_ok = false;
    for (unsigned attempt = 0; !job->cancel.load(); ++attempt) {
      job->retry_attempt.store(attempt);
      job->retry_seconds_left.store(0);
      job->bytes_now.store(0);
      job->bytes_total.store(0);

      std::string geojson;
      int64_t wire_bytes = 0;

      /* The progress callback runs on this worker thread (inside curl).
         It updates the per-slice byte counters and returns false when
         cancel is set, which causes curl to abort the transfer. */
      auto progress = [&job](uint64_t now, uint64_t total) -> bool {
        if (job->cancel.load())
          return false;
        job->bytes_now.store(now);
        job->bytes_total.store(total);
        return true;
      };

      const bool ok = api.DownloadGeoJSON(
        job->param, slot_date, slot_run_hour, slot_step,
        geojson, &wire_bytes, progress);

      if (job->cancel.load())
        break;

      if (ok) {
        job->total_wire_bytes.fetch_add((uint64_t)wire_bytes);
        job->total_disk_bytes.fetch_add((uint64_t)geojson.size());
        job->succeeded_or_cached.fetch_add(1);
        job->newly_downloaded.fetch_add(1);

        {
          std::lock_guard lock{job->result_mutex};
          if (job->first_forecast.IsEmpty()) {
            auto forecast = XCThermGeoJSON::Parse(geojson, true);
            if (!forecast.IsEmpty()) {
              forecast.layer_name = job->target_label;
              job->first_forecast = std::move(forecast);
            }
          }
        }
        slot_ok = true;
        break;
      }

      /* Download failed — sleep ~5 s, checking cancel every 100 ms so
         Stop responds quickly even mid-retry. */
      constexpr unsigned RETRY_SECONDS = 5;
      LogFmt("xctherm: +{}h attempt {} failed — retrying in {}s",
             offset, attempt + 1, RETRY_SECONDS);
      for (unsigned s = RETRY_SECONDS; s > 0 && !job->cancel.load(); --s) {
        job->retry_seconds_left.store(s);
        for (int t = 0; t < 10 && !job->cancel.load(); ++t)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      job->retry_seconds_left.store(0);
    }

    if (!slot_ok && !job->cancel.load())
      job->any_slot_missing.store(true);
    }

    job->finished_at = std::chrono::steady_clock::now();
  } catch (...) {
    const std::lock_guard lock{job->result_mutex};
    job->error = std::current_exception();
  }

  job->done.store(true);
}

void
XCThermWidget::SpanClicked()
{
  static constexpr const char *choices[] = {
    "1 hour", "3 hours", "6 hours", "12 hours", "18 hours",
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

  /* Seed the cursor to the currently active layer — but only here,
     once, on initial display. UpdateList() deliberately does NOT touch
     the cursor on subsequent calls so a user who downloads layer X
     while layer Y is active stays parked on X after the download. */
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  size_t count = 0;
  const auto *layers = GetLayers((unsigned)settings.model, count);
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
  auto *widget_ptr =
    static_cast<XCThermWidget *>(&buttons->GetWidget());
  widget_ptr->SetButtonPanel(*buttons);
  return buttons;
}

#endif
