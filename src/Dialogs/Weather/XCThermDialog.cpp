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
#include "Weather/xctherm/LayerDownloadStatus.hpp"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "Weather/xctherm/XCThermDownloadGlue.hpp"
#include "Weather/xctherm/XCThermDownloadJob.hpp"
#include "Weather/xctherm/XCThermForecastTime.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#include "net/http/Init.hpp"

#include <memory>

namespace {

static constexpr StaticEnumChoice span_list[] = {
  { 1, N_("1 hour") },
  { 3, N_("3 hours") },
  { 6, N_("6 hours") },
  { 12, N_("12 hours") },
  { 18, N_("18 hours") },
  nullptr
};

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
          XCTherm::FormatLayerStatus(settings.model, selected_layer,
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
  overlay.Reload(PageLayout::Overlay::XCTHERM,
                 apply_to_page_button, add_page_button);
  UpdateTimeControl();
  UpdateAltitudeControl();
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
  overlay.ApplyClicked(apply_to_page_button, add_page_button);
  UpdateTimeControl();
  UpdateAltitudeControl();
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

  XCTherm::MarkLayerPending(settings.model, selected_layer, span_hours);

  SyncUpdateButtons();
  UpdateStatusControl();
  poll_timer.Schedule(std::chrono::milliseconds(200));
}

void
XCThermWidget::PollDownload() noexcept
{
  if (!active_job)
    return;

  auto *info = XCTherm::GetLayerDownloadInfo(active_job->model);
  XCTherm::CopyJobProgress(info[active_job->target_index], *active_job);

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

  const unsigned span = job->span_hours;
  const unsigned ok = job->succeeded_or_cached.load();
  const unsigned nu = job->newly_downloaded.load();
  const bool canceled = job->cancel.load();
  const auto result = XCTherm::ApplyFinishedDownload(job);

  SyncUpdateButtons();
  UpdateStatusControl();
  UpdateTimeControl();
  UpdateAltitudeControl();

  if (result == XCTherm::FinishDownloadResult::FAILED) {
    if (job->index_no_parameters.load())
      ShowMessageBox(_("Forecast index has no XC Therm parameters."),
                     "XC Therm", MB_OK);
    else if (job->error_eptr)
      ShowError(job->error_eptr, "XC Therm");
    else
      ShowMessageBox(_("Forecast download failed.\nKeeping previous data."),
                     "XC Therm", MB_OK);
    return;
  }

  if (result == XCTherm::FinishDownloadResult::CANCELED)
    return;

  if (nu > 0)
    PageActions::Update();

  if (job->error_eptr && !canceled)
    ShowError(job->error_eptr, "XC Therm");
  else if (result == XCTherm::FinishDownloadResult::PARTIAL) {
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

  auto *info = XCTherm::GetLayerDownloadInfo(settings.model);
  info[selected_layer] = XCTherm::LayerDownloadInfo{};

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
  XCTherm::RehydrateLayerRowsFromCache(settings.model);
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

  auto *layer = AddEnum(C_("Weather control", "Layer"),
                        _("Altitude layer used for Update and Delete. "
                          "Use Altitude below to change what the map shows."));
  layer->GetDataField()->SetOnModified([this]{
    OnLayerModified();
  });

  AddReadOnly(_("Status"),
              _("Download and cache status for the selected layer."));

  AddEnum(C_("Weather control", "Span"),
          _("How many forecast hours to download with Update."),
          span_list, settings.download_span_hours);
  GetControl(SPAN).GetDataField()->SetOnModified([this]{
    OnSpanModified();
  });

  update_button = AddButton(_("Update"), [this]{ DownloadClicked(); });
  delete_button = AddButton(C_("Button", "Delete"), [this]{ DeleteClicked(); });
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

  WeatherOverlayDraft::AddApplyAddButtons(*this, apply_to_page_button,
                                          add_page_button,
                                          [this]{ ApplyToPageClicked(); },
                                          [this]{ AddPageClicked(); });
  AddSpacer();

  WeatherOverlayDraft::AddPagesSetupButton(*this, [this]{
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
