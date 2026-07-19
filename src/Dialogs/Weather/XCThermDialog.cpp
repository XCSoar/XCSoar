// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "WeatherOverlayDraft.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "UIState.hpp"
#include "Language/Language.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/FieldControls.hpp"
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
 * Download metadata for a layer — shown in the Status row.
 * For a span download, sizes/speed are totals across all hourly slices,
 * span_hours is the number of slices successfully fetched, and
 * pending_index/pending_total animate progress during the loop.
 */
struct LayerDownloadInfo {
  enum Status { NONE, PENDING, DONE, FAILED, CANCELED };
  Status status = NONE;
  double wire_mb = 0.0;
  double speed_mbs = 0.0;
  unsigned span_hours = 0;
  unsigned future_hours = 0;
  unsigned new_downloads = 0;
  unsigned pending_index = 0;
  unsigned pending_total = 0;
  uint64_t pending_bytes_now = 0;
  uint64_t pending_bytes_total = 0;
  unsigned retry_attempt = 0;
  unsigned retry_seconds_left = 0;
  std::string download_time;
  std::string issued_utc;
};

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

static constexpr StaticEnumChoice span_list[] = {
  { 1, N_("1 hour") },
  { 3, N_("3 hours") },
  { 6, N_("6 hours") },
  { 12, N_("12 hours") },
  { 18, N_("18 hours") },
  nullptr
};

StaticString<200>
FormatLayerStatus(unsigned model, unsigned layer_index,
                  unsigned span_hours_setting) noexcept
{
  StaticString<200> text;
  const auto &region = XCTherm::GetRegion(model);
  if (layer_index >= region.layer_count) {
    text = _("None");
    return text;
  }

  const auto *info = GetDownloadInfo(model);
  switch (info[layer_index].status) {
  case LayerDownloadInfo::PENDING: {
    const auto &p = info[layer_index];
    if (p.retry_seconds_left > 0)
      text.Format(_("Slot %u: reconnect in %us (try #%u)"),
                  p.pending_index,
                  p.retry_seconds_left,
                  p.retry_attempt + 1);
    else if (p.pending_bytes_total > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      const double tot_mb = (double)p.pending_bytes_total / (1024.0 * 1024.0);
      text.Format(_("Slot %u/%u: %.2f / %.2f MB"),
                  p.pending_index, p.pending_total, now_mb, tot_mb);
    } else if (p.pending_bytes_now > 0) {
      const double now_mb = (double)p.pending_bytes_now / (1024.0 * 1024.0);
      text.Format(_("Slot %u/%u: %.2f MB"),
                  p.pending_index, p.pending_total, now_mb);
    } else if (p.pending_total > 0)
      text.Format(_("Slot %u/%u: connecting..."),
                  p.pending_index, p.pending_total);
    else
      text = _("Connecting...");
    break;
  }
  case LayerDownloadInfo::DONE: {
    const unsigned future = info[layer_index].future_hours;
    if (info[layer_index].new_downloads == 0)
      text.Format(_("%u/%uh | Issued %s | %s"),
                  future, span_hours_setting,
                  info[layer_index].issued_utc.c_str(),
                  info[layer_index].download_time.c_str());
    else
      text.Format(_("%u/%uh (%u new) | Issued %s | %.2f MB wire "
                    "%.1f MB/s | %s"),
                  future, span_hours_setting,
                  info[layer_index].new_downloads,
                  info[layer_index].issued_utc.c_str(),
                  info[layer_index].wire_mb,
                  info[layer_index].speed_mbs,
                  info[layer_index].download_time.c_str());
    break;
  }
  case LayerDownloadInfo::FAILED:
    text = _("Download failed");
    break;
  case LayerDownloadInfo::CANCELED:
    text = _("Cancelled");
    break;
  default:
    text = _("Not downloaded");
    break;
  }

  return text;
}

class XCThermWidget final : public RowFormWidget {
  enum Controls {
    LAYER,
    STATUS,
    SPAN,
    UPDATE,
    DELETE_BUTTON,
    SPACER_AFTER_UPDATE,
    TIME,
    ALTITUDE,
    APPLY_TO_PAGE,
    ADD_PAGE,
    SPACER_AFTER_ADD,
  };

  Button *update_button = nullptr;
  Button *delete_button = nullptr;
  Button *apply_to_page_button = nullptr;
  Button *add_page_button = nullptr;

  unsigned selected_layer = 0;
  WeatherOverlayDraft::State overlay;

  std::shared_ptr<XCThermDownloadJob> active_job;
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};

  static XCThermWidget *active;

public:
  XCThermWidget() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  ~XCThermWidget() noexcept override {
    if (auto *glue = GetXCThermDownloadGlue())
      glue->Abandon();
    if (active == this)
      active = nullptr;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Unprepare() noexcept override;

private:
  void SaveSettings() noexcept;
  void UpdateLayerControl() noexcept;
  void UpdateStatusControl() noexcept;
  void UpdateSpanControl() noexcept;
  void SyncUpdateButtons() noexcept;
  void UpdateTimeControl() noexcept;
  void UpdateAltitudeControl() noexcept;
  void RefreshPageSection() noexcept;
  void OnLayerModified() noexcept;
  void OnSpanModified() noexcept;
  void ApplyToPageClicked() noexcept;
  void AddPageClicked() noexcept;
  bool EditTime(DataField &df) noexcept;
  bool EditAltitude(DataField &df) noexcept;

  void DownloadClicked() noexcept;
  void DeleteClicked() noexcept;
  void StartDownload() noexcept;
  void PollDownload() noexcept;
  void FinishDownload() noexcept;
  void CancelDownload() noexcept;
  void RehydrateRowsFromCache() noexcept;

  static bool EditTimeCallback(const char *caption, DataField &df,
                               const char *help_text) noexcept;
  static bool EditAltitudeCallback(const char *caption, DataField &df,
                                   const char *help_text) noexcept;
};

XCThermWidget *XCThermWidget::active = nullptr;

void
XCThermWidget::SaveSettings() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  Profile::Set(ProfileKeys::XCThermModel, (int)settings.model);
  Profile::Set(ProfileKeys::XCThermParameter, (int)settings.parameter);
  Profile::Set(ProfileKeys::XCThermWaveHeight, (int)settings.wave_height);
  Profile::Set(ProfileKeys::XCThermVerticalWindAGL,
               (int)settings.vertical_wind_agl);
}

void
XCThermWidget::UpdateLayerControl() noexcept
{
  auto &control = GetControl(LAYER);
  auto &df = (DataFieldEnum &)*control.GetDataField();
  df.ClearChoices();

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);

  for (unsigned i = 0; i < region.layer_count; ++i)
    df.AddChoice(i, gettext(region.layers[i].dialog_label));

  if (selected_layer >= region.layer_count)
    selected_layer = 0;

  if (region.layer_count == 0) {
    df.AddChoice(unsigned(-1), _("None"));
    df.SetValue(unsigned(-1));
    control.SetEnabled(false);
  } else {
    df.SetValue(selected_layer);
    control.SetEnabled(!active_job);
  }

  control.RefreshDisplay();
}

void
XCThermWidget::UpdateStatusControl() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  SetText(STATUS,
          FormatLayerStatus(settings.model, selected_layer,
                            settings.download_span_hours).c_str());
}

void
XCThermWidget::UpdateSpanControl() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  LoadValueEnum(SPAN, settings.download_span_hours);
  GetControl(SPAN).SetEnabled(!active_job);
  GetControl(SPAN).RefreshDisplay();
}

void
XCThermWidget::SyncUpdateButtons() noexcept
{
  const bool job_running = (bool)active_job;
  if (update_button != nullptr) {
    update_button->SetCaption(job_running ? _("Stop") : _("Update"));
    update_button->SetEnabled(true);
  }
  if (delete_button != nullptr)
    delete_button->SetEnabled(!job_running);

  GetControl(LAYER).SetEnabled(!job_running);
  GetControl(SPAN).SetEnabled(!job_running);
}

void
XCThermWidget::UpdateTimeControl() noexcept
{
  StaticString<64> label;
  XCTherm::FormatTimeLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(TIME), label.c_str(), true);
}

void
XCThermWidget::UpdateAltitudeControl() noexcept
{
  StaticString<64> label;
  XCTherm::FormatLayerLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(ALTITUDE), label.c_str(),
                                    true);
}

void
XCThermWidget::RefreshPageSection() noexcept
{
  overlay.Load(PageLayout::Overlay::XCTHERM);
  UpdateTimeControl();
  UpdateAltitudeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
XCThermWidget::OnLayerModified() noexcept
{
  auto &df = (DataFieldEnum &)*GetControl(LAYER).GetDataField();
  const unsigned value = df.GetValue();
  if (value == unsigned(-1))
    return;

  /* Layer is Update/Delete only — do not write into overlay settings
     or the live map cursor (Altitude / page controls own that). */
  selected_layer = value;
  UpdateStatusControl();
}

void
XCThermWidget::OnSpanModified() noexcept
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  settings.download_span_hours = GetValueEnum(SPAN);
  SaveSettings();
  UpdateStatusControl();
}

void
XCThermWidget::ApplyToPageClicked() noexcept
{
  if (!overlay.ApplyIfDirty())
    return;

  UpdateTimeControl();
  UpdateAltitudeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
XCThermWidget::AddPageClicked() noexcept
{
  overlay.AddPage(apply_to_page_button, add_page_button);
}

bool
XCThermWidget::EditTime([[maybe_unused]] DataField &df) noexcept
{
  if (!XCTherm::EditTimeOnLayout(overlay.draft))
    return true;

  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
XCThermWidget::EditAltitude([[maybe_unused]] DataField &df) noexcept
{
  const auto result = XCTherm::EditLayerOnLayout(overlay.draft, false);
  if (result == XCTherm::LayerPickerResult::OPEN_SETUP)
    return false;
  if (result != XCTherm::LayerPickerResult::CHANGED)
    return true;

  UpdateAltitudeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
XCThermWidget::EditTimeCallback([[maybe_unused]] const char *caption,
                                DataField &df,
                                [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditTime(df) : false;
}

bool
XCThermWidget::EditAltitudeCallback([[maybe_unused]] const char *caption,
                                    DataField &df,
                                    [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditAltitude(df) : false;
}

void
XCThermWidget::DownloadClicked() noexcept
{
  if (active_job) {
    CancelDownload();
    return;
  }
  StartDownload();
}

void
XCThermWidget::CancelDownload() noexcept
{
  if (!active_job)
    return;

  if (auto *glue = GetXCThermDownloadGlue())
    glue->RequestCancel();
  else
    active_job->cancel.store(true);
}

void
XCThermWidget::StartDownload() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  const unsigned span_hours = settings.download_span_hours;
  if (span_hours == 0)
    return;

  const auto &region = XCTherm::GetRegion(settings.model);
  if (selected_layer >= region.layer_count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }

  XCThermAPI::Instance().PrepareSession(settings);

  active_job = XCTherm::StartSpanDownload(
    settings, selected_layer,
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
  auto &row_info = info[selected_layer];
  row_info.status = LayerDownloadInfo::PENDING;
  row_info.pending_total = span_hours + 1;
  row_info.pending_index = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

  SyncUpdateButtons();
  UpdateStatusControl();
  poll_timer.Schedule(std::chrono::milliseconds(200));
}

void
XCThermWidget::PollDownload() noexcept
{
  if (!active_job)
    return;

  auto &job = *active_job;
  auto *info = GetDownloadInfo(job.model);
  auto &row_info = info[job.target_index];
  row_info.pending_index = job.current_offset.load();
  row_info.pending_total = job.span_hours + 1;
  row_info.pending_bytes_now = job.bytes_now.load();
  row_info.pending_bytes_total = job.bytes_total.load();
  row_info.retry_attempt = job.retry_attempt.load();
  row_info.retry_seconds_left = job.retry_seconds_left.load();

  UpdateStatusControl();
}

void
XCThermWidget::FinishDownload() noexcept
{
  if (!active_job)
    return;

  poll_timer.Cancel();

  auto job = std::move(active_job);
  active_job.reset();

  auto *info = GetDownloadInfo(job->model);
  auto &row_info = info[job->target_index];

  const unsigned span = job->span_hours;
  const unsigned ok = job->succeeded_or_cached.load();
  const unsigned nu = job->newly_downloaded.load();
  const bool canceled = job->cancel.load();
  const bool any_miss = job->any_slot_missing.load();

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
    SyncUpdateButtons();
    UpdateStatusControl();
    if (!canceled) {
      if (job->index_no_parameters.load()) {
        ShowMessageBox(_("Forecast index has no XCTherm parameters."),
                       "XCTherm", MB_OK);
      } else if (job->error_eptr) {
        ShowError(job->error_eptr, "XCTherm");
      } else {
        ShowMessageBox(_("Forecast download failed.\nKeeping previous data."),
                       "XCTherm", MB_OK);
      }
    }
    return;
  }

  XCTherm::ApplyJobPreviewToMap(job);

  const double span_secs = std::chrono::duration<double>(
    std::chrono::steady_clock::now() - job->started_at).count();
  const double wire_mb =
    (double)job->total_wire_bytes.load() / (1024.0 * 1024.0);
  const double speed_mbs = span_secs > 0 ? wire_mb / span_secs : 0.0;

  row_info.status = LayerDownloadInfo::DONE;
  row_info.wire_mb = wire_mb;
  row_info.speed_mbs = speed_mbs;
  row_info.span_hours = ok;
  row_info.new_downloads = nu;
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
  if (lt != nullptr && std::strftime(tbuf, sizeof(tbuf), "%H:%M:%S", lt) > 0)
    row_info.download_time = tbuf;

  if (nu > 0) {
    const unsigned current_utc = XCTherm::GetUtcTimeParts().hour;
    const unsigned dropped =
      XCThermAPI::Instance().PruneStaleRuns(job->param, current_utc);
    if (dropped > 0)
      LogFmt("xctherm: stale-run sweep dropped {} entries for {}",
             dropped, job->param);
  }

  SyncUpdateButtons();
  UpdateStatusControl();
  UpdateTimeControl();
  UpdateAltitudeControl();

  if (nu > 0)
    PageActions::Update();

  if (job->error_eptr && !canceled) {
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
XCThermWidget::DeleteClicked() noexcept
{
  if (active_job)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);

  if (selected_layer >= region.layer_count) {
    ShowMessageBox(_("No layer selected."), "XCTherm", MB_OK);
    return;
  }

  const auto &target = region.layers[selected_layer];
  XCThermAPI::Instance().ClearLayer(target.api_parameter);

  auto *info = GetDownloadInfo(settings.model);
  info[selected_layer] = LayerDownloadInfo{};

  /* Clear the map only when the deleted layer is what the live cursor
     (or legacy activated settings) is showing — not the Layer row. */
  const auto &weather = CommonInterface::GetUIState().weather;
  const bool clears_live_overlay =
    weather.xctherm.cursor_initialized
      ? weather.xctherm_cursor.layer == selected_layer
      : XCTherm::IsActiveLayer(target, settings.parameter,
                               settings.wave_height,
                               settings.vertical_wind_agl);
  if (clears_live_overlay)
    XCTherm::ClearMapOverlay();

  UpdateStatusControl();
  UpdateTimeControl();
  UpdateAltitudeControl();
  PageActions::Update();
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
  RowFormWidget::Prepare(parent, rc);
  active = this;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const int active_layer = XCTherm::FindActiveLayerIndex(settings);
  selected_layer = active_layer >= 0 ? unsigned(active_layer) : 0;

  auto *layer = AddEnum(_("Layer"),
                        _("Altitude layer used for Update and Delete. "
                          "Use Altitude below to change what the map shows."));
  layer->GetDataField()->SetOnModified([this]{
    OnLayerModified();
  });

  AddReadOnly(_("Status"),
              _("Download and cache status for the selected layer."));

  AddEnum(_("Span"),
          _("How many forecast hours to download with Update."),
          span_list, settings.download_span_hours);
  GetControl(SPAN).GetDataField()->SetOnModified([this]{
    OnSpanModified();
  });

  update_button = AddButton(_("Update"), [this]{ DownloadClicked(); });
  delete_button = AddButton(_("Delete"), [this]{ DeleteClicked(); });
  AddSpacer();

  StaticString<256> time_help;
  time_help.Format(_("Forecast time for the current map page %s overlay. "
                     "Opens the same picker as the weather controls "
                     "(Auto, Now, or a UTC hour)."),
                   "XCTherm");
  auto *time = AddEnum(_("Time"), time_help.c_str());
  time->SetEditCallback(EditTimeCallback);

  auto *altitude = AddEnum(_("Altitude"),
                           _("Altitude band for the current map page. "
                             "Use Apply to page to commit changes."));
  altitude->SetEditCallback(EditAltitudeCallback);

  apply_to_page_button = AddButton(_("Apply to page"), [this]{
    ApplyToPageClicked();
  });
  add_page_button = AddButton(_("Add page"), [this]{
    AddPageClicked();
  });
  AddSpacer();

  AddButton(_("Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
    RefreshPageSection();
  });
}

void
XCThermWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  XCThermAPI::Instance().PrepareSession(
    CommonInterface::GetComputerSettings().weather.xctherm);
  RehydrateRowsFromCache();

  UpdateLayerControl();
  UpdateStatusControl();
  UpdateSpanControl();
  SyncUpdateButtons();
  RefreshPageSection();
}

void
XCThermWidget::Hide() noexcept
{
  WindowWidget::Hide();
}

void
XCThermWidget::Unprepare() noexcept
{
  if (active == this)
    active = nullptr;
  update_button = nullptr;
  delete_button = nullptr;
  RowFormWidget::Unprepare();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermMainWidget() noexcept
{
  return std::make_unique<XCThermWidget>();
}

#endif
