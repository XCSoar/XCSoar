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

#include "Dialogs/GroupedListPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
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
#include <vector>

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
    text = C_("Status", "Cancelled");
    break;
  default:
    text = _("Not downloaded");
    break;
  }

  return text;
}

/**
 * The XC Therm weather page: which layer to download, how that
 * download is going, and the time and altitude of the page the map
 * shows now.
 */
class XCThermWidget final : public GroupedListWidget {
  unsigned selected_layer = 0;
  WeatherOverlayDraft::State overlay;

  std::shared_ptr<XCThermDownloadJob> active_job;
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};

public:
  XCThermWidget() noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()) {}

  ~XCThermWidget() noexcept override {
    if (auto *glue = GetXCThermDownloadGlue())
      glue->Abandon();
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;

private:
  void SaveSettings() noexcept;
  void Refresh() noexcept;
  void Fill() noexcept;
  void PickLayer() noexcept;
  void PickSpan() noexcept;

  void DownloadClicked() noexcept;
  void DeleteClicked() noexcept;
  void StartDownload() noexcept;
  void PollDownload() noexcept;
  void FinishDownload() noexcept;
  void CancelDownload() noexcept;
  void RehydrateRowsFromCache() noexcept;
};

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
XCThermWidget::Refresh() noexcept
{
  Clear();
  Fill();

  /* the layer, the span and the status are filled by their
     callbacks.  A layout before that leaves them blank, and a
     status with no text is drawn as Disabled */
  UpdateValues();
  UpdateLayout();
}

void
XCThermWidget::PickLayer() noexcept
{
  if (active_job)
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);
  if (region.layer_count == 0)
    return;

  std::vector<PickerChoice> choices;
  choices.reserve(region.layer_count);
  for (unsigned i = 0; i < region.layer_count; ++i)
    choices.push_back({gettext(region.layers[i].dialog_label)});

  const int current = selected_layer < region.layer_count
    ? (int)selected_layer : -1;
  const int picked = PickChoice(C_("Weather control", "Layer"),
                                _("Altitude layer used for Update and Delete. "
                                  "Use Altitude below to change what the map "
                                  "shows."),
                                choices, current);
  if (picked < 0)
    return;

  /* Layer is Update/Delete only — do not write into overlay settings
     or the live map cursor (Altitude / page controls own that). */
  selected_layer = (unsigned)picked;
  UpdateValues();
}

void
XCThermWidget::PickSpan() noexcept
{
  if (active_job)
    return;

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  unsigned span = settings.download_span_hours;
  if (!PickEnum(C_("Weather control", "Span"),
                _("How many forecast hours to download with Update."),
                span_list, span))
    return;

  settings.download_span_hours = span;
  SaveSettings();
  UpdateValues();
}

void
XCThermWidget::Fill() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);
  if (selected_layer >= region.layer_count)
    selected_layer = 0;

  AddGroup();

  AddValue(C_("Weather control", "Layer"),
           _("Altitude layer used for Update and Delete. "
             "Use Altitude below to change what the map shows."),
           [this](ValueState &state) {
             const auto &live =
               CommonInterface::GetComputerSettings().weather.xctherm;
             const auto &live_region = XCTherm::GetRegion(live.model);
             state.disabled = (bool)active_job;
             if (live_region.layer_count == 0 ||
                 selected_layer >= live_region.layer_count) {
               state.text = _("None");
               state.disabled = true;
               return;
             }

             state.text =
               gettext(live_region.layers[selected_layer].dialog_label);
           },
           [this]{ PickLayer(); });

  AddValue(_("Status"),
           _("Download and cache status for the selected layer."),
           [this](ValueState &state) {
             const auto &live =
               CommonInterface::GetComputerSettings().weather.xctherm;
             const auto status =
               FormatLayerStatus(live.model, selected_layer,
                                 live.download_span_hours);
             state.text = status.c_str();
           });

  AddValue(C_("Weather control", "Span"),
           _("How many forecast hours to download with Update."),
           [this](ValueState &state) {
             const auto &live =
               CommonInterface::GetComputerSettings().weather.xctherm;
             state.text = GetEnumCaption(span_list,
                                         live.download_span_hours);
             state.disabled = (bool)active_job;
           },
           [this]{ PickSpan(); });

  const bool job_running = (bool)active_job;
  AddButtonRow({
    {job_running ? _("Stop") : _("Update"),
     [this]{ DownloadClicked(); }, false},
    {C_("Button", "Delete"), [this]{ DeleteClicked(); }, job_running},
  });

  const auto &ui_state = CommonInterface::GetUIState();
  const auto &ui_settings = CommonInterface::GetUISettings();
  const unsigned page_index = ui_state.pages.current_index;
  const PageLayout &page = ui_settings.pages.pages[page_index];

  StaticString<64> title_buffer;
  const char *title =
    page.MakeTitle(ui_settings.info_boxes,
                   std::span{title_buffer.data(), title_buffer.capacity()});

  StaticString<128> caption;
  caption.Format("%s %u: %s", _("Page"), page_index + 1, title);
  AddGroup(caption);

  StaticString<256> time_help;
  time_help.Format(_("Forecast time for the current map page %s overlay. "
                     "Opens the same picker as the weather controls "
                     "(Auto, Now, or a UTC hour)."),
                   "XC Therm");

  AddValue(C_("Weather control", "Time"), time_help.c_str(),
           [this](ValueState &state) {
             StaticString<64> label;
             XCTherm::FormatTimeLabelForPage(label, overlay.draft);
             state.text = label.c_str();
           },
           [this]{
             if (XCTherm::EditTimeOnLayout(overlay.draft))
               Refresh();
           });

  AddValue(C_("Weather control", "Altitude"),
           _("Altitude band for the current map page. "
             "Use Apply to page to commit changes."),
           [this](ValueState &state) {
             StaticString<64> label;
             XCTherm::FormatLayerLabelForPage(label, overlay.draft);
             state.text = label.c_str();
           },
           [this]{
             const auto result =
               XCTherm::EditLayerOnLayout(overlay.draft, false);
             if (result == XCTherm::LayerPickerResult::CHANGED)
               Refresh();
           });

  AddButtonRow({
    {C_("Button", "Apply to page"), [this]{
      if (overlay.ApplyIfDirty())
        Refresh();
    }, !overlay.IsDirty()},
    {C_("Button", "Add page"), [this]{
      overlay.AddPage(nullptr, nullptr);
      Refresh();
    }, !overlay.CanAddPage()},
  });

  AddGroup();
  AddButton(C_("Button", "Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
    overlay.Load(PageLayout::Overlay::XCTHERM);
    Refresh();
  });
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
    ShowMessageBox(_("No layer selected."), "XC Therm", MB_OK);
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
      ShowMessageBox(_("Network is not available."), "XC Therm", MB_OK);
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

  Refresh();
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

  UpdateValues();
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
    Refresh();
    if (!canceled) {
      if (job->index_no_parameters.load()) {
        ShowMessageBox(_("Forecast index has no XC Therm parameters."),
                       "XC Therm", MB_OK);
      } else if (job->error_eptr) {
        ShowError(job->error_eptr, "XC Therm");
      } else {
        ShowMessageBox(_("Forecast download failed.\nKeeping previous data."),
                       "XC Therm", MB_OK);
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

  Refresh();

  if (nu > 0)
    PageActions::Update();

  if (job->error_eptr && !canceled) {
    ShowError(job->error_eptr, "XC Therm");
  } else if (any_miss && !canceled) {
    StaticString<128> msg;
    msg.Format(_("Got %u of %u hourly slices (%u newly downloaded).\n"
                 "Some slots were unavailable."),
               ok, span, nu);
    ShowMessageBox(msg, "XC Therm", MB_OK);
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
    ShowMessageBox(_("No layer selected."), "XC Therm", MB_OK);
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

  UpdateValues();
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
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const int active_layer = XCTherm::FindActiveLayerIndex(settings);
  selected_layer = active_layer >= 0 ? unsigned(active_layer) : 0;

  overlay.Load(PageLayout::Overlay::XCTHERM);
  Fill();
  GroupedListWidget::Prepare(parent, rc);
}

void
XCThermWidget::Show(const PixelRect &rc) noexcept
{
  GroupedListWidget::Show(rc);

  XCThermAPI::Instance().PrepareSession(
    CommonInterface::GetComputerSettings().weather.xctherm);
  RehydrateRowsFromCache();

  overlay.Load(PageLayout::Overlay::XCTHERM);
  Refresh();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermMainWidget() noexcept
{
  return std::make_unique<XCThermWidget>();
}

#endif
