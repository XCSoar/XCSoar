// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "OverlayPageActions.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "UIState.hpp"
#include "Language/Language.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "Weather/xctherm/XCThermDownloadGlue.hpp"
#include "Weather/xctherm/XCThermDownloadJob.hpp"
#include "Weather/xctherm/XCThermForecastTime.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#include "LogFile.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "net/http/Init.hpp"

#include <chrono>
#include <ctime>
#include <memory>

namespace {

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
static LayerDownloadInfo download_info_ch[
  XCTherm::GetRegion(XCTherm::Region::CH).layer_count];
static LayerDownloadInfo download_info_uk[
  XCTherm::GetRegion(XCTherm::Region::UK).layer_count];

static LayerDownloadInfo *
GetDownloadInfo(unsigned model) noexcept
{
  if (XCTherm::ToRegion(model) == XCTherm::Region::UK)
    return download_info_uk;
  return download_info_ch;
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
  const auto &region = XCTherm::GetRegion(model);
  if (index >= region.layer_count)
    return;

  const auto &layer = region.layers[index];
  const bool active = XCTherm::IsActiveLayer(layer, active_parameter,
                                             active_wave_height,
                                             active_vertical_agl);

  StaticString<80> first_row;
  if (active)
    first_row.Format("%s  %s", gettext(layer.dialog_label), _("[ACTIVE]"));
  else
    first_row = gettext(layer.dialog_label);

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
  Button *add_to_current_button = nullptr;
  Button *add_to_new_page_button = nullptr;
  Button *download_button = nullptr;   // labeled "Update" or "Stop"
  Button *delete_button = nullptr;

  XCThermRowRenderer row_renderer;

  std::shared_ptr<XCThermDownloadJob> active_job;

  /* Polls active_job every 200 ms for live progress (UI thread). */
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

  ~XCThermWidget() noexcept override {
    if (auto *glue = GetXCThermDownloadGlue())
      glue->Abandon();
  }

private:
  void UpdateList();
  void SaveSettings();
  bool SetActiveLayerFromCursor();

  void ActivateClicked();
  void AddToCurrentClicked();
  void AddToNewPageClicked();
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
     Region (CH/UK) lives in Config → Weather → XCTherm. */
  span_button = buttons.Add(_("Span"), [this]() { SpanClicked(); });
  activate_button = buttons.Add(_("Activate"),
                                [this]() { ActivateClicked(); });
  add_to_current_button = buttons.Add(_("Add to page"),
                                      [this]() { AddToCurrentClicked(); });
  add_to_new_page_button = buttons.Add(_("Add new page"),
                                       [this]() { AddToNewPageClicked(); });
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

  const auto &region = XCTherm::GetRegion(settings.model);

  list.SetLength(region.layer_count);

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

  const auto &region = XCTherm::GetRegion(settings.model);

  const bool cursor_is_active = index < region.layer_count &&
    XCTherm::IsActiveLayer(region.layers[index], settings.parameter,
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
  AddToCurrentClicked();
}

bool
XCThermWidget::SetActiveLayerFromCursor()
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  const int index = GetList().GetCursorIndex();
  const auto &region = XCTherm::GetRegion(settings.model);
  if (index < 0 || unsigned(index) >= region.layer_count)
    return false;

  const auto &layer = region.layers[index];
  if (layer.is_agl) {
    settings.parameter = 1;
    settings.vertical_wind_agl = layer.altitude_m;
  } else {
    settings.parameter = 0;
    settings.wave_height = layer.altitude_m;
  }

  return true;
}

void
XCThermWidget::AddToCurrentClicked()
{
  if (!SetActiveLayerFromCursor())
    return;

  SaveSettings();
  CommonInterface::SetUIState().weather.xctherm.cursor_initialized = false;
  WeatherDialogOverlayActions::AddOverlayToCurrentPage(
    PageLayout::Overlay::XCTHERM);
  UpdateList();
}

void
XCThermWidget::AddToNewPageClicked()
{
  if (!SetActiveLayerFromCursor())
    return;

  SaveSettings();
  CommonInterface::SetUIState().weather.xctherm.cursor_initialized = false;
  WeatherDialogOverlayActions::AddOverlayToNewPage(
    PageLayout::Overlay::XCTHERM);

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

  const auto &region = XCTherm::GetRegion(settings.model);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || unsigned(cursor_index) >= region.layer_count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }
  const auto &target = region.layers[cursor_index];

  XCThermAPI::Instance().ClearLayer(target.api_parameter);

  /* Reset the per-row status so the dialog stops showing the old
     "Cached Nh | Issued …" line. */
  auto *info = GetDownloadInfo(settings.model);
  info[cursor_index] = LayerDownloadInfo{};

  /* If the deleted layer was currently displayed on the map, clear the
     overlay and refresh page state. */
  if (XCTherm::IsActiveLayer(target, settings.parameter,
                             settings.wave_height,
                             settings.vertical_wind_agl))
    XCTherm::ClearMapOverlay();

  UpdateList();
  PageActions::Update();
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

  if (auto *glue = GetXCThermDownloadGlue())
    glue->RequestCancel();
  else if (active_job)
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
  const auto &region = XCTherm::GetRegion(settings.model);

  const int cursor_index = GetList().GetCursorIndex();
  if (cursor_index < 0 || unsigned(cursor_index) >= region.layer_count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }
  XCThermAPI::Instance().PrepareSession(settings);

  active_job = XCTherm::StartSpanDownload(
    settings, unsigned(cursor_index),
    [this](std::shared_ptr<XCThermDownloadJob> finished) {
      active_job = std::move(finished);
      FinishDownload();
    });
  if (active_job == nullptr) {
    if (GetXCThermDownloadGlue() == nullptr || Net::curl == nullptr)
      ShowMessageBox(_("Network is not available."), "XCTherm", MB_OK);
    return;
  }

  auto *info = GetDownloadInfo(settings.model);
  auto &row_info = info[cursor_index];
  row_info.status = LayerDownloadInfo::PENDING;
  row_info.pending_total = span_hours + 1;
  row_info.pending_index = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

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
    } else if (job->index_no_parameters.load()) {
      ShowMessageBox(_("Forecast index has no XCTherm parameters."),
                     "XCTherm", MB_OK);
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

  XCTherm::ApplyJobPreviewToMap(job);

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
    const unsigned current_utc = XCTherm::GetUtcTimeParts().hour;

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
  const auto &region = XCTherm::GetRegion(settings.model);
  auto *info = GetDownloadInfo(settings.model);
  auto &api = XCThermAPI::Instance();

  /* Make sure the disk index is built before we read it — otherwise a
     fresh session that opens this dialog without having downloaded
     anything yet would show "Not downloaded" for slices that are in
     fact sitting in the on-disk cache. Idempotent. */
  api.EnableDiskCache();

  for (unsigned i = 0; i < region.layer_count; ++i) {
    /* Don't overwrite session state — a row that's mid-download
       (PENDING) or failed/canceled should keep its visible status. */
    if (info[i].status != LayerDownloadInfo::NONE)
      continue;

    const auto summary =
      api.GetCachedLayerSummary(region.layers[i].api_parameter);
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
  const auto &region = XCTherm::GetRegion(settings.model);
  for (unsigned i = 0; i < region.layer_count; ++i) {
    if (XCTherm::IsActiveLayer(region.layers[i], settings.parameter,
                               settings.wave_height,
                               settings.vertical_wind_agl)) {
      GetList().SetCursorIndex(i);
      break;
    }
  }
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermMainWidget() noexcept
{
  auto widget = std::make_unique<XCThermWidget>();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(widget),
    ButtonPanelWidget::Alignment::BOTTOM);
  auto *widget_ptr = static_cast<XCThermWidget *>(&buttons->GetWidget());
  widget_ptr->SetButtonPanel(*buttons);
  return buttons;
}

#endif
