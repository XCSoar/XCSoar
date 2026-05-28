// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapOverlayControlsWidget.hpp"

#include "Blackboard/BlackboardListener.hpp"
#include "Dialogs/Message.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "Weather/MapOverlay/EdlControlsModel.hpp"
#include "Weather/MapOverlay/RaspControlsModel.hpp"
#include "Weather/MapOverlay/Usage.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/EDL/TileStore.hpp"
#ifdef HAVE_EDL
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#endif
#include "Widget/RowFormWidget.hpp"
#include "Widget/SolidWidget.hpp"
#include "util/StaticString.hxx"

#include <memory>
#include <vector>

class MapOverlayControlsWidget final
  : public RowFormWidget,
    private DataFieldListener,
    NullBlackboardListener
#ifdef HAVE_EDL
  , private EDL::DownloadListener
#endif
{
  const MapOverlay::Usage usage;
  const PageLayout::Overlay overlay;
  MapOverlay::EdlControlsModel edl;
  MapOverlay::RaspControlsModel rasp;
  std::vector<EDL::CachedDay> cached_days;

  unsigned auto_advance_row = unsigned(-1);
  unsigned forecast_row = unsigned(-1);
  unsigned level_row = unsigned(-1);
  unsigned rasp_time_row = unsigned(-1);
  unsigned cached_day_row = unsigned(-1);
  Button *precache_day_button = nullptr;
  Button *clean_other_days_button = nullptr;
  bool blackboard_listener_registered = false;
#ifdef HAVE_EDL
  EDL::DownloadGlue *edl_listener_glue = nullptr;
#endif

public:
  MapOverlayControlsWidget(MapOverlay::Usage _usage,
                           PageLayout::Overlay _overlay) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     usage(_usage),
     overlay(_overlay) {}

  ~MapOverlayControlsWidget() noexcept override {
    UnregisterBlackboardListener();
#ifdef HAVE_EDL
    UnregisterEdlDownloadListener();
#endif
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Unprepare() noexcept override;

private:
  void RegisterBlackboardListener() noexcept;
  void UnregisterBlackboardListener() noexcept;
#ifdef HAVE_EDL
  void UnregisterEdlDownloadListener() noexcept;
#endif
  void RefreshControls() noexcept;
  void UpdateAutoAdvanceControls() noexcept;

  [[gnu::pure]]
  bool GetAutoAdvance() const noexcept;

  void SetAutoAdvance(bool auto_advance) noexcept;
  void ApplyAutoAdvance() noexcept;
  void RefreshEdlOverlay();
  void PrecacheDay();
  void CleanOtherDays();

  void OnModified(DataField &df) noexcept override;
  void OnGPSUpdate(const MoreData &basic) override;

#ifdef HAVE_EDL
  void OnDownloadFinished(const EDL::DownloadNotification &notification) noexcept override;
#endif
};

void
MapOverlayControlsWidget::RegisterBlackboardListener() noexcept
{
  if (blackboard_listener_registered ||
      usage != MapOverlay::Usage::MAP_BOTTOM ||
      overlay != PageLayout::Overlay::EDL)
    return;

  CommonInterface::GetLiveBlackboard().AddListener(*this);
  blackboard_listener_registered = true;
}

void
MapOverlayControlsWidget::UnregisterBlackboardListener() noexcept
{
  if (!blackboard_listener_registered)
    return;

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  blackboard_listener_registered = false;
}

#ifdef HAVE_EDL
void
MapOverlayControlsWidget::UnregisterEdlDownloadListener() noexcept
{
  if (edl_listener_glue == nullptr)
    return;

  edl_listener_glue->RemoveListener(*this);
  edl_listener_glue = nullptr;
}
#endif

void
MapOverlayControlsWidget::UpdateAutoAdvanceControls() noexcept
{
  if (auto_advance_row == unsigned(-1))
    return;

  const bool auto_advance = GetAutoAdvance();
  LoadValue(auto_advance_row, auto_advance);
  GetControl(auto_advance_row).RefreshDisplay();

  if (forecast_row != unsigned(-1))
    SetRowEnabled(forecast_row, !auto_advance);

  if (level_row != unsigned(-1))
    SetRowEnabled(level_row, !auto_advance);

  if (rasp_time_row != unsigned(-1))
    SetRowEnabled(rasp_time_row, !auto_advance);
}

bool
MapOverlayControlsWidget::GetAutoAdvance() const noexcept
{
  switch (overlay) {
  case PageLayout::Overlay::EDL:
    return edl.GetForecastAutoAdvance();

  case PageLayout::Overlay::RASP:
    return rasp.GetTimeAutoAdvance();

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }

  return false;
}

void
MapOverlayControlsWidget::SetAutoAdvance(bool auto_advance) noexcept
{
  switch (overlay) {
  case PageLayout::Overlay::EDL:
    edl.SetForecastAutoAdvance(auto_advance);
    break;

  case PageLayout::Overlay::RASP:
    rasp.SetTimeAutoAdvance(auto_advance);
    break;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }
}

void
MapOverlayControlsWidget::ApplyAutoAdvance() noexcept
{
  switch (overlay) {
  case PageLayout::Overlay::EDL:
    EDL::UpdateCurrentLevel();
    {
      const auto &basic = CommonInterface::Basic();
      if (basic.date_time_utc.IsPlausible())
        EDL::OnTimeUpdate(basic.date_time_utc);
      else
        EDL::OnTimeUpdate(BrokenDateTime::NowUTC());
    }
    RefreshEdlOverlay();
    break;

  case PageLayout::Overlay::RASP:
    rasp.ApplyAutoAdvanceTime();
    RefreshControls();
    break;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;
  }
}

void
MapOverlayControlsWidget::RefreshControls() noexcept
{
  if (forecast_row != unsigned(-1)) {
    auto &control = GetControl(forecast_row);
    edl.FillForecastChoices((DataFieldEnum &)*control.GetDataField());
    control.RefreshDisplay();
  }

  UpdateAutoAdvanceControls();

  if (level_row != unsigned(-1)) {
    auto &control = GetControl(level_row);
    edl.FillLevelChoices((DataFieldEnum &)*control.GetDataField());
    control.RefreshDisplay();
  }

  if (rasp_time_row != unsigned(-1)) {
    auto &control = GetControl(rasp_time_row);
    rasp.FillTimeChoices((DataFieldEnum &)*control.GetDataField(),
                         DataGlobals::GetRasp());
    control.RefreshDisplay();
  }

  if (cached_day_row != unsigned(-1)) {
    auto &control = GetControl(cached_day_row);
    auto &df = (DataFieldEnum &)*control.GetDataField();
    df.ClearChoices();

    cached_days = EDL::ListDownloadedDays();
    if (cached_days.empty()) {
      df.AddChoice(-1, _("None"));
      df.SetValue(-1);
      control.SetEnabled(false);
      if (clean_other_days_button != nullptr)
        clean_other_days_button->SetEnabled(false);
    } else {
      control.SetEnabled(true);
      if (clean_other_days_button != nullptr)
        clean_other_days_button->SetEnabled(true);

      const unsigned selected_index =
        edl.SelectedCachedDayIndex(cached_days);
      for (unsigned i = 0; i < cached_days.size(); ++i)
        df.AddChoice(i, edl.FormatCachedDayLabel(cached_days[i]).c_str());

      df.SetValue(selected_index);
    }

    control.RefreshDisplay();
  }

  if (precache_day_button != nullptr)
    precache_day_button->SetEnabled(EDL::OverlayEnabled());
}

void
MapOverlayControlsWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  unsigned next_row = 0;

  if (usage == MapOverlay::Usage::MAP_BOTTOM) {
    switch (overlay) {
    case PageLayout::Overlay::EDL:
      auto_advance_row = next_row++;
      AddBoolean(_("Auto advance"),
                 _("Automatically follow the next UTC forecast hour and the next lower pressure level below the aircraft."),
                 true, this);

      forecast_row = next_row++;
      AddEnum(_("Forecast"), nullptr, this);

      level_row = next_row++;
      AddEnum(_("Level"), nullptr, this);
      break;

    case PageLayout::Overlay::RASP:
      auto_advance_row = next_row++;
      AddBoolean(_("Auto advance"),
                 _("Automatically follow the current local time in 15-minute steps."),
                 true, this);

      rasp_time_row = next_row++;
      AddEnum(_("Forecast"), nullptr, this);
      break;

    case PageLayout::Overlay::NONE:
    case PageLayout::Overlay::MAX:
      break;
    }
  }

  if (usage == MapOverlay::Usage::SETTINGS &&
      overlay == PageLayout::Overlay::EDL) {
    cached_day_row = next_row++;
    AddEnum(_("Cached day"), nullptr, this);

#ifdef HAVE_HTTP
    precache_day_button = AddButton(_("Precache day"), [this]{ PrecacheDay(); });
#endif

    clean_other_days_button = AddButton(_("Clean other days"),
                                        [this]{ CleanOtherDays(); });
  }
}

void
MapOverlayControlsWidget::Show(const PixelRect &rc) noexcept
{
  if (overlay == PageLayout::Overlay::RASP)
    rasp.SyncFromPageLayout();

  const bool refresh_edl = edl.OnShow(usage, overlay);

  RowFormWidget::Show(rc);

  RefreshControls();

  RegisterBlackboardListener();

#ifdef HAVE_EDL
  if (overlay == PageLayout::Overlay::EDL &&
      net_components != nullptr && net_components->edl != nullptr) {
    edl_listener_glue = net_components->edl.get();
    edl_listener_glue->AddListener(*this);
  }
#endif

  if (refresh_edl)
    RefreshEdlOverlay();
}

void
MapOverlayControlsWidget::Hide() noexcept
{
  EDL::ClearGpsUiRefreshPending();
  UnregisterBlackboardListener();

#ifdef HAVE_EDL
  UnregisterEdlDownloadListener();
#endif

  WindowWidget::Hide();
}

void
MapOverlayControlsWidget::OnModified(DataField &df) noexcept
{
  if (auto_advance_row != unsigned(-1) &&
      IsDataField(auto_advance_row, df)) {
    SetAutoAdvance(GetValueBoolean(auto_advance_row));
    UpdateAutoAdvanceControls();

    if (GetAutoAdvance())
      ApplyAutoAdvance();

    return;
  }

  if (forecast_row != unsigned(-1) && IsDataField(forecast_row, df)) {
    edl.SelectForecast(GetValueEnum(forecast_row));
    UpdateAutoAdvanceControls();
    RefreshEdlOverlay();
    return;
  }

  if (level_row != unsigned(-1) && IsDataField(level_row, df)) {
    edl.SelectLevel(GetValueEnum(level_row));
    UpdateAutoAdvanceControls();
    RefreshEdlOverlay();
    return;
  }

  if (rasp_time_row != unsigned(-1) && IsDataField(rasp_time_row, df)) {
    rasp.SetTime(GetValueEnum(rasp_time_row));
    UpdateAutoAdvanceControls();
    RefreshControls();
  }
}

void
MapOverlayControlsWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  if (overlay != PageLayout::Overlay::EDL ||
      !edl.GetForecastAutoAdvance())
    return;

  if (EDL::TakeGpsUiRefreshPending())
    RefreshEdlOverlay();
}

void
MapOverlayControlsWidget::Unprepare() noexcept
{
  EDL::ClearGpsUiRefreshPending();
  UnregisterBlackboardListener();
  auto_advance_row = unsigned(-1);
  forecast_row = unsigned(-1);
  level_row = unsigned(-1);
  rasp_time_row = unsigned(-1);
  cached_day_row = unsigned(-1);
  precache_day_button = nullptr;
  clean_other_days_button = nullptr;
  cached_days.clear();
  RowFormWidget::Unprepare();
}

void
MapOverlayControlsWidget::RefreshEdlOverlay()
{
#if !defined(HAVE_HTTP) || !defined(HAVE_EDL)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  if (!EDL::OverlayEnabled()) {
    RefreshControls();
    return;
  }

  RefreshControls();
  EDL::RequestOverlayRefresh();
#endif
}

#ifdef HAVE_EDL
void
MapOverlayControlsWidget::OnDownloadFinished(
  const EDL::DownloadNotification &) noexcept
{
  RefreshControls();
}
#endif

void
MapOverlayControlsWidget::PrecacheDay()
{
#if !defined(HAVE_HTTP) || !defined(HAVE_EDL)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  if (!EDL::OverlayEnabled())
    return;

  EDL::RequestPrecacheDay(EDL::GetForecastTime());
#endif
}

void
MapOverlayControlsWidget::CleanOtherDays()
{
  if (cached_days.empty())
    return;

  const auto selected_index = GetValueEnum(cached_day_row);
  if (selected_index >= cached_days.size())
    return;

  StaticString<96> message;
  message.Format(_("Keep only %04u-%02u-%02u and delete the other cached days?"),
                 cached_days[selected_index].day.year,
                 cached_days[selected_index].day.month,
                 cached_days[selected_index].day.day);
  if (ShowMessageBox(message, _("Weather"), MB_YESNO) != IDYES)
    return;

  const unsigned deleted =
    EDL::DeleteOtherDownloadedDays(cached_days[selected_index].day);
  RefreshControls();

  StaticString<64> result;
  result.Format(_("Deleted %u cached files."), deleted);
  ShowMessageBox(result, _("Weather"), MB_OK);
}

std::unique_ptr<Widget>
CreateMapOverlayControlsOverlayWidget(PageLayout::Overlay overlay) noexcept
{
  return std::make_unique<MapOverlayControlsWidget>(
    MapOverlay::Usage::SETTINGS, overlay);
}

std::unique_ptr<Widget>
CreateMapOverlayControlsBottomWidget(PageLayout::Overlay overlay) noexcept
{
  return std::make_unique<SolidWidget>(
    std::make_unique<MapOverlayControlsWidget>(
      MapOverlay::Usage::MAP_BOTTOM, overlay));
}
