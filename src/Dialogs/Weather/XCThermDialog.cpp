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
#include "Form/CheckBox.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TwoWidgets.hpp"
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

#include <algorithm>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>
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

static void
ResetAllDownloadInfo() noexcept
{
  for (auto &info : download_info_ch)
    info = LayerDownloadInfo{};
  for (auto &info : download_info_uk)
    info = LayerDownloadInfo{};
}

static constexpr StaticEnumChoice span_list[] = {
  { 1, N_("1 hour") },
  { 3, N_("3 hours") },
  { 6, N_("6 hours") },
  { 12, N_("12 hours") },
  { 18, N_("18 hours") },
  nullptr
};

static bool
LayerParameterInList(std::string_view parameter,
                       std::string_view list) noexcept
{
  while (!list.empty()) {
    const auto comma = list.find(',');
    const std::string_view token = list.substr(0, comma);
    if (token == parameter)
      return true;
    if (comma == std::string_view::npos)
      break;
    list.remove_prefix(comma + 1);
  }
  return false;
}

/**
 * Restore checkbox selection from the profile (or the active overlay
 * layer if nothing was saved yet).
 */
static void
LoadSelectedLayersFromProfile(unsigned model,
                              MultiSelectListWidget &list) noexcept
{
  const auto &region = XCTherm::GetRegion(model);
  list.SetLengthWithSelection(region.layer_count);

  const char *configured =
    Profile::Get(ProfileKeys::XCThermSelectedLayers);
  if (configured != nullptr && *configured != '\0') {
    const std::string_view saved{configured};
    bool any = false;
    for (unsigned i = 0; i < region.layer_count; ++i) {
      if (!LayerParameterInList(region.layers[i].api_parameter, saved))
        continue;
      list.SetSelected(i, true);
      any = true;
    }
    if (any)
      return;
  }

  const int active = XCTherm::FindActiveLayerIndex(
    CommonInterface::GetComputerSettings().weather.xctherm);
  if (active >= 0)
    list.SetSelected(unsigned(active), true);
}

static void
SaveSelectedLayersToProfile(unsigned model,
                            const MultiSelectListWidget &list) noexcept
{
  const auto &region = XCTherm::GetRegion(model);
  /* Must clear: StaticString default-ctors leave the buffer
     uninitialized, so append() can start mid-garbage and the saved
     list is truncated or corrupted (checkboxes look "random"). */
  StaticString<512> value;
  value.clear();
  bool first = true;
  for (unsigned i = 0; i < region.layer_count; ++i) {
    if (!list.IsSelected(i))
      continue;
    if (!first)
      value += ',';
    first = false;
    value += region.layers[i].api_parameter;
  }
  Profile::Set(ProfileKeys::XCThermSelectedLayers, value.c_str());
  /* Flush so selections survive process kill / relaunch on Android. */
  Profile::Save();
}

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

class XCThermOptionsPanel final : public RowFormWidget {
  enum Controls {
    SPAN,
    DELETE_BUTTON,
    SPACER_AFTER_DELETE,
    TIME,
    ALTITUDE,
    APPLY_TO_PAGE,
    ADD_PAGE,
    SPACER_AFTER_ADD,
  };

  Button *apply_to_page_button = nullptr;
  Button *add_page_button = nullptr;
  WeatherOverlayDraft::State overlay;

  static XCThermOptionsPanel *active;

public:
  XCThermOptionsPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  ~XCThermOptionsPanel() noexcept override {
    if (active == this)
      active = nullptr;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  void OnDownloadActivityChanged() noexcept {
    if (!controls_ready)
      return;

    GetControl(SPAN).SetEnabled(active_job_count == 0);
    GetControl(SPAN).RefreshDisplay();
    if (delete_button != nullptr)
      delete_button->SetEnabled(active_job_count == 0);
  }

  void RefreshPageSection() noexcept {
    if (!controls_ready)
      return;

    UpdateTimeControl();
    UpdateAltitudeControl();
    overlay.SyncButtons(apply_to_page_button, add_page_button);
  }

  void SetActiveJobCount(unsigned count) noexcept {
    active_job_count = count;
    OnDownloadActivityChanged();
  }

private:
  Button *delete_button = nullptr;
  unsigned active_job_count = 0;
  bool controls_ready = false;

  void SaveSettings() noexcept;
  void UpdateSpanControl() noexcept;
  void OnSpanModified() noexcept;
  void ApplyToPageClicked() noexcept;
  void AddPageClicked() noexcept;
  void DeleteAllClicked() noexcept;
  bool EditTime(DataField &df) noexcept;
  bool EditAltitude(DataField &df) noexcept;
  void UpdateTimeControl() noexcept;
  void UpdateAltitudeControl() noexcept;

  static bool EditTimeCallback(const char *caption, DataField &df,
                               const char *help_text) noexcept;
  static bool EditAltitudeCallback(const char *caption, DataField &df,
                                   const char *help_text) noexcept;
};

XCThermOptionsPanel *XCThermOptionsPanel::active = nullptr;

class XCThermLayerListWidget final : public MultiSelectListWidget {
  TwoTextRowsRenderer row_renderer;
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *preload_button = nullptr;
  XCThermOptionsPanel *options_panel = nullptr;

  std::shared_ptr<XCThermDownloadJob> active_job;
  std::vector<unsigned> preload_queue;
  UI::PeriodicTimer poll_timer{[this]{ PollDownload(); }};
  bool loading_selection = false;

public:
  static XCThermLayerListWidget *active;

  XCThermLayerListWidget() noexcept = default;

  ~XCThermLayerListWidget() noexcept override {
    if (auto *glue = GetXCThermDownloadGlue())
      glue->Abandon();
    if (active == this)
      active = nullptr;
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons_widget = &_buttons;
  }

  void SetOptionsPanel(XCThermOptionsPanel &_options) noexcept {
    options_panel = &_options;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Unprepare() noexcept override;
  void InvalidateList() noexcept;

private:
  void CreateButtons(ButtonPanel &buttons) noexcept;
  void SyncButtons() noexcept;
  void SaveSelection() noexcept;
  void LoadSelection() noexcept;
  void RehydrateRowsFromCache() noexcept;
  void PreloadSelectedClicked() noexcept;
  void DownloadClicked() noexcept;
  void CancelDownload() noexcept;
  void StartDownload(unsigned layer_index) noexcept;
  void StartNextQueuedDownload() noexcept;
  void PollDownload() noexcept;
  void FinishDownload() noexcept;

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

protected:
  void OnSelectionChanged() noexcept override {
    if (loading_selection)
      return;
    SaveSelection();
    SyncButtons();
  }
};

XCThermLayerListWidget *XCThermLayerListWidget::active = nullptr;

void
XCThermOptionsPanel::SaveSettings() noexcept
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
XCThermOptionsPanel::UpdateSpanControl() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  LoadValueEnum(SPAN, settings.download_span_hours);
  GetControl(SPAN).SetEnabled(active_job_count == 0);
  GetControl(SPAN).RefreshDisplay();
}

void
XCThermOptionsPanel::OnSpanModified() noexcept
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  settings.download_span_hours = GetValueEnum(SPAN);
  SaveSettings();
}

void
XCThermOptionsPanel::ApplyToPageClicked() noexcept
{
  if (!overlay.ApplyIfDirty())
    return;

  UpdateTimeControl();
  UpdateAltitudeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
XCThermOptionsPanel::AddPageClicked() noexcept
{
  overlay.AddPage(apply_to_page_button, add_page_button);
}

void
XCThermOptionsPanel::UpdateTimeControl() noexcept
{
  StaticString<64> label;
  XCTherm::FormatTimeLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(TIME), label.c_str(), true);
}

void
XCThermOptionsPanel::UpdateAltitudeControl() noexcept
{
  StaticString<64> label;
  XCTherm::FormatLayerLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(ALTITUDE), label.c_str(),
                                    true);
}

bool
XCThermOptionsPanel::EditTime([[maybe_unused]] DataField &df) noexcept
{
  if (!XCTherm::EditTimeOnLayout(overlay.draft))
    return true;

  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
XCThermOptionsPanel::EditAltitude([[maybe_unused]] DataField &df) noexcept
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
XCThermOptionsPanel::EditTimeCallback([[maybe_unused]] const char *caption,
                                      DataField &df,
                                      [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditTime(df) : false;
}

bool
XCThermOptionsPanel::EditAltitudeCallback([[maybe_unused]] const char *caption,
                                          DataField &df,
                                          [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditAltitude(df) : false;
}

void
XCThermOptionsPanel::DeleteAllClicked() noexcept
{
  if (active_job_count > 0)
    return;

  if (ShowMessageBox(_("Delete all cached XC Therm forecast data? "
                       "This includes every altitude layer and region."),
                     _("XC Therm"), MB_YESNO | MB_ICONWARNING) != IDYES)
    return;

  XCThermAPI::Instance().ClearAllCachedData();
  ResetAllDownloadInfo();
  XCTherm::ClearMapOverlay();

  if (XCThermLayerListWidget::active != nullptr)
    XCThermLayerListWidget::active->InvalidateList();

  RefreshPageSection();
  PageActions::Update();
}

void
XCThermOptionsPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);
  active = this;

  AddEnum(C_("Weather control", "Span"),
          _("How many forecast hours to download for each selected layer."),
          span_list,
          CommonInterface::GetComputerSettings().weather.xctherm
            .download_span_hours);
  GetControl(SPAN).GetDataField()->SetOnModified([this]{
    OnSpanModified();
  });

  delete_button = AddButton(C_("Button", "Delete"), [this]{
    DeleteAllClicked();
  });
  AddSpacer();

  StaticString<256> time_help;
  time_help.Format(_("Forecast time for the current map page %s overlay. "
                     "Opens the same picker as the weather controls "
                     "(Auto, Now, or a UTC hour)."),
                   "XC Therm");
  auto *time = AddEnum(C_("Weather control", "Time"), time_help.c_str());
  time->SetEditCallback(EditTimeCallback);

  auto *altitude = AddEnum(C_("Weather control", "Altitude"),
                           _("Altitude band for the current map page. "
                             "Use Apply to page to commit changes."));
  altitude->SetEditCallback(EditAltitudeCallback);

  apply_to_page_button = AddButton(C_("Button", "Apply to page"), [this]{
    ApplyToPageClicked();
  });
  add_page_button = AddButton(C_("Button", "Add page"), [this]{
    AddPageClicked();
  });
  AddSpacer();

  AddButton(C_("Button", "Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
    overlay.Load(PageLayout::Overlay::XCTHERM);
    RefreshPageSection();
  });

  controls_ready = true;
  OnDownloadActivityChanged();
}

void
XCThermOptionsPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  overlay.Load(PageLayout::Overlay::XCTHERM);
  UpdateSpanControl();
  RefreshPageSection();
}

void
XCThermOptionsPanel::Unprepare() noexcept
{
  if (active == this)
    active = nullptr;
  controls_ready = false;
  delete_button = nullptr;
  apply_to_page_button = nullptr;
  add_page_button = nullptr;
  RowFormWidget::Unprepare();
}

void
XCThermLayerListWidget::CreateButtons(ButtonPanel &buttons) noexcept
{
  preload_button = buttons.Add(C_("Button", "Preload Selected"), [this]() {
    PreloadSelectedClicked();
  });
  buttons.Add(_("Stop"), [this]() {
    DownloadClicked();
  });
  buttons.EnableCursorSelection();
}

void
XCThermLayerListWidget::SyncButtons() noexcept
{
  if (preload_button == nullptr)
    return;

  const bool job_running = (bool)active_job;
  preload_button->SetEnabled(!job_running &&
                             GetSelectedCount() > 0);

  if (options_panel != nullptr)
    options_panel->SetActiveJobCount(job_running ? 1u : 0u);
}

void
XCThermLayerListWidget::SaveSelection() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  SaveSelectedLayersToProfile(settings.model, *this);
}

void
XCThermLayerListWidget::LoadSelection() noexcept
{
  loading_selection = true;
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  LoadSelectedLayersFromProfile(settings.model, *this);
  loading_selection = false;
  SyncButtons();
}

void
XCThermLayerListWidget::InvalidateList() noexcept
{
  GetList().Invalidate();
  SyncButtons();
}

void
XCThermLayerListWidget::RehydrateRowsFromCache() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);
  auto *info = GetDownloadInfo(settings.model);
  auto &api = XCThermAPI::Instance();

  api.EnableDiskCache();

  for (unsigned i = 0; i < region.layer_count; ++i) {
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
XCThermLayerListWidget::PreloadSelectedClicked() noexcept
{
  if (active_job)
    return;

  const auto selected = GetSelectedIndices();
  if (selected.empty()) {
    ShowMessageBox(_("No layers selected."), "XC Therm", MB_OK);
    return;
  }

  StaticString<256> prompt;
  prompt.Format(_("Download %u selected XC Therm layers for offline use?"),
                unsigned(selected.size()));
  if (ShowMessageBox(prompt.c_str(), _("XC Therm"), MB_YESNO | MB_ICONQUESTION)
      != IDYES)
    return;

  preload_queue = selected;
  StartNextQueuedDownload();
}

void
XCThermLayerListWidget::DownloadClicked() noexcept
{
  if (active_job)
    CancelDownload();
}

void
XCThermLayerListWidget::CancelDownload() noexcept
{
  if (!active_job)
    return;

  preload_queue.clear();

  if (auto *glue = GetXCThermDownloadGlue())
    glue->RequestCancel();
  else
    active_job->cancel.store(true);
}

void
XCThermLayerListWidget::StartNextQueuedDownload() noexcept
{
  while (!preload_queue.empty()) {
    const unsigned layer_index = preload_queue.front();
    preload_queue.erase(preload_queue.begin());
    StartDownload(layer_index);
    if (active_job)
      return;

    /* Start failed (e.g. network unavailable).  Stop the queue so the
       user does not get one error dialog per selected layer. */
    preload_queue.clear();
    return;
  }
}

void
XCThermLayerListWidget::StartDownload(unsigned layer_index) noexcept
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
  if (layer_index >= region.layer_count)
    return;

  XCThermAPI::Instance().PrepareSession(settings);

  active_job = XCTherm::StartSpanDownload(
    settings, layer_index,
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
  auto &row_info = info[layer_index];
  row_info.status = LayerDownloadInfo::PENDING;
  row_info.pending_total = span_hours + 1;
  row_info.pending_index = 0;
  row_info.pending_bytes_now = 0;
  row_info.pending_bytes_total = 0;
  row_info.retry_attempt = 0;
  row_info.retry_seconds_left = 0;

  SyncButtons();
  InvalidateList();
  poll_timer.Schedule(std::chrono::milliseconds(200));
}

void
XCThermLayerListWidget::PollDownload() noexcept
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

  InvalidateList();
}

void
XCThermLayerListWidget::FinishDownload() noexcept
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
    SyncButtons();
    InvalidateList();

    preload_queue.clear();

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

  SyncButtons();
  InvalidateList();

  if (options_panel != nullptr)
    options_panel->RefreshPageSection();

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

  if (!canceled && !preload_queue.empty())
    StartNextQueuedDownload();
}

void
XCThermLayerListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                    unsigned idx) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = XCTherm::GetRegion(settings.model);
  if (idx >= region.layer_count)
    return;

  const DialogLook &look = UIGlobals::GetDialogLook();
  const bool focused = GetList().HasFocus();
  const unsigned padding = Layout::GetTextPadding();
  const unsigned box_size = rc.GetHeight() > 2 * padding
    ? rc.GetHeight() - 2 * padding
    : 0;

  PixelRect box_rc;
  box_rc.left = rc.left + (int)padding;
  box_rc.top = rc.top + (int)padding;
  box_rc.right = box_rc.left + (int)box_size;
  box_rc.bottom = box_rc.top + (int)box_size;

  DrawCheckBox(canvas, look, box_rc, IsSelected(idx), focused, false, true);

  PixelRect text_rc = rc;
  text_rc.left = box_rc.right + 2 * (int)padding;

  row_renderer.DrawFirstRow(canvas, text_rc,
                            gettext(region.layers[idx].dialog_label));
  row_renderer.DrawSecondRow(
    canvas, text_rc,
    FormatLayerStatus(settings.model, idx,
                      settings.download_span_hours).c_str());
}

void
XCThermLayerListWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  active = this;

  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));

  LoadSelection();

  MultiSelectListWidget::Prepare(parent, rc);

  if (buttons_widget != nullptr)
    CreateButtons(buttons_widget->GetButtonPanel());
}

void
XCThermLayerListWidget::Show(const PixelRect &rc) noexcept
{
  MultiSelectListWidget::Show(rc);

  XCThermAPI::Instance().PrepareSession(
    CommonInterface::GetComputerSettings().weather.xctherm);
  RehydrateRowsFromCache();

  LoadSelection();
  InvalidateList();

  if (options_panel != nullptr)
    options_panel->RefreshPageSection();
}

void
XCThermLayerListWidget::Hide() noexcept
{
  SaveSelection();
  MultiSelectListWidget::Hide();
}

void
XCThermLayerListWidget::Unprepare() noexcept
{
  poll_timer.Cancel();

  if (active == this)
    active = nullptr;
  preload_button = nullptr;
  DeleteWindow();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermMainWidget() noexcept
{
  auto list = std::make_unique<XCThermLayerListWidget>();
  auto *list_ptr = list.get();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(list),
    ButtonPanelWidget::Alignment::BOTTOM);
  list_ptr->SetButtonPanel(*buttons);

  auto options = std::make_unique<XCThermOptionsPanel>();
  auto *options_ptr = options.get();
  list_ptr->SetOptionsPanel(*options_ptr);

  return std::make_unique<TwoWidgets>(std::move(buttons),
                                      std::move(options));
}

#endif
