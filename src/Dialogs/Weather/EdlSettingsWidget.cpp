// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlSettingsWidget.hpp"

#include "Dialogs/Message.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/EDL/TileStore.hpp"
#ifdef HAVE_EDL
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#endif
#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"

#include <memory>
#include <vector>

namespace {

unsigned
SelectedCachedDayIndex(const std::vector<EDL::CachedDay> &days) noexcept
{
  if (days.empty())
    return 0;

  const auto current_day = EDL::GetForecastTime().AtMidnight();
  for (unsigned i = 0; i < days.size(); ++i)
    if (days[i].day == current_day)
      return i;

  return 0;
}

StaticString<40>
FormatCachedDayLabel(const EDL::CachedDay &day) noexcept
{
  StaticString<40> label;
  label.Format("%04u-%02u-%02u (%s, %u)",
               day.day.year, day.day.month, day.day.day,
               day.IsComplete() ? _("Complete") : _("Partial"),
               day.file_count);
  return label;
}

} // namespace

class EdlSettingsWidget final
  : public RowFormWidget
#ifdef HAVE_EDL
  , private EDL::DownloadListener
#endif
{
  std::vector<EDL::CachedDay> cached_days;

  unsigned cached_day_row = unsigned(-1);
  Button *precache_day_button = nullptr;
  Button *clean_other_days_button = nullptr;
#ifdef HAVE_EDL
  EDL::DownloadGlue *edl_listener_glue = nullptr;
#endif

public:
  EdlSettingsWidget() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  ~EdlSettingsWidget() noexcept override {
#ifdef HAVE_EDL
    UnregisterEdlDownloadListener();
#endif
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Unprepare() noexcept override;

private:
#ifdef HAVE_EDL
  void UnregisterEdlDownloadListener() noexcept;
#endif
  void RefreshControls();
  void PrecacheDay();
  void CleanOtherDays();

#ifdef HAVE_EDL
  void OnDownloadFinished(const EDL::DownloadNotification &notification) noexcept override;
#endif
};

#ifdef HAVE_EDL
void
EdlSettingsWidget::UnregisterEdlDownloadListener() noexcept
{
  if (edl_listener_glue == nullptr)
    return;

  edl_listener_glue->RemoveListener(*this);
  edl_listener_glue = nullptr;
}
#endif

void
EdlSettingsWidget::RefreshControls()
{
  if (cached_day_row != unsigned(-1)) {
    auto &control = GetControl(cached_day_row);
    auto &df = (DataFieldEnum &)*control.GetDataField();
    df.ClearChoices();

    try {
      cached_days = EDL::ListDownloadedDays();
    } catch (...) {
      cached_days.clear();
    }

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
        SelectedCachedDayIndex(cached_days);
      for (unsigned i = 0; i < cached_days.size(); ++i)
        df.AddChoice(i, FormatCachedDayLabel(cached_days[i]).c_str());

      df.SetValue(selected_index);
    }

    control.RefreshDisplay();
  }

  if (precache_day_button != nullptr)
    precache_day_button->SetEnabled(EDL::OverlayEnabled());
}

void
EdlSettingsWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  cached_day_row = 0;
  AddEnum(_("Cached day"), nullptr);

#ifdef HAVE_HTTP
  precache_day_button = AddButton(_("Precache day"), [this]{ PrecacheDay(); });
#endif

  clean_other_days_button = AddButton(_("Clean other days"),
                                      [this]{ CleanOtherDays(); });
}

void
EdlSettingsWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  RefreshControls();

#ifdef HAVE_EDL
  if (net_components != nullptr && net_components->edl != nullptr) {
    edl_listener_glue = net_components->edl.get();
    edl_listener_glue->AddListener(*this);
  }
#endif
}

void
EdlSettingsWidget::Hide() noexcept
{
#ifdef HAVE_EDL
  UnregisterEdlDownloadListener();
#endif

  WindowWidget::Hide();
}

void
EdlSettingsWidget::Unprepare() noexcept
{
  cached_day_row = unsigned(-1);
  precache_day_button = nullptr;
  clean_other_days_button = nullptr;
  cached_days.clear();
  RowFormWidget::Unprepare();
}

void
EdlSettingsWidget::PrecacheDay()
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

#ifdef HAVE_EDL
void
EdlSettingsWidget::OnDownloadFinished(
  const EDL::DownloadNotification &) noexcept
{
  RefreshControls();
}
#endif

void
EdlSettingsWidget::CleanOtherDays()
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
CreateEdlSettingsWidget() noexcept
{
  return std::make_unique<EdlSettingsWidget>();
}
