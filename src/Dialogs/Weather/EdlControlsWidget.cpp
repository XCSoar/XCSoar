// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsWidget.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Form/SelectorButtonsWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "Screen/Layout.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#ifdef HAVE_HTTP
#include "net/http/Init.hpp"
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/TileStore.hpp"
#endif
#include "Weather/EDL/StateController.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/dim/Size.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"

#include <array>
#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>

#include "Form/Panel.hpp"

class EDLAlwaysShowWidget final : public WindowWidget {
  std::function<void()> on_enabled;

public:
  explicit EDLAlwaysShowWidget(std::function<void()> _on_enabled) noexcept
    :on_enabled(std::move(_on_enabled)) {}

  PixelSize GetMinimumSize() const noexcept override
  {
    const unsigned height = Layout::GetMinimumControlHeight();
    return {
      CheckBoxControl::GetMinimumWidth(UIGlobals::GetDialogLook(), height,
                                       _("Always show EDL on map")),
      height,
    };
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    WindowStyle style;
    style.Hide();
    style.TabStop();

    auto checkbox = std::make_unique<CheckBoxControl>();
    checkbox->Create(parent, UIGlobals::GetDialogLook(),
                     _("Always show EDL on map"), rc, style,
                     [this](bool enabled){
                       EDL::SetShowOnMainMap(enabled);
                       if (enabled)
                         on_enabled();
                       else
                         EDL::RefreshOverlayVisibility();
                     });
    SetWindow(std::move(checkbox));
  }

  void Show(const PixelRect &rc) noexcept override
  {
    if (!IsDefined())
      return;

    auto &checkbox = (CheckBoxControl &)GetWindow();
    checkbox.SetState(EDL::ShouldShowOnMainMap());
    WindowWidget::Show(rc);
  }

  void RefreshState() noexcept
  {
    if (!IsDefined())
      return;

    auto &checkbox = (CheckBoxControl &)GetWindow();
    checkbox.SetState(EDL::ShouldShowOnMainMap());
  }
};

class ForecastRowHandler final : public SelectorButtonsWidget::Handler {
  static constexpr unsigned forecast_choices = 24;

  std::array<BrokenDateTime, forecast_choices> choices{};
  std::function<void(BrokenDateTime)> on_selected;
  std::function<void(int)> on_step;

public:
  ForecastRowHandler(std::function<void(int)> _on_step,
                     std::function<void(BrokenDateTime)> _on_selected) noexcept
    :on_selected(std::move(_on_selected)),
     on_step(std::move(_on_step)) {}

  const char *GetLabel() const noexcept override
  {
    return _("Forecast");
  }

  void FillChoices(DataFieldEnum &field) noexcept override
  {
    EDL::EnsureInitialised();
    const auto selected_time = EDL::GetForecastTime();
    if (!selected_time.IsPlausible())
      return;

    field.ClearChoices();

    const auto base_time = selected_time + std::chrono::hours{-11};
    unsigned selected_index = 0;
    for (unsigned i = 0; i < forecast_choices; ++i) {
      choices[i] = base_time + std::chrono::hours{i};
      const auto local_time = LocalTime(choices[i].ToTimePoint());

      StaticString<32> label;
      label.Format("%02u:00", unsigned(local_time.tm_hour));
      field.AddChoice(i, label.c_str());

      if (choices[i] == selected_time)
        selected_index = i;
    }

    field.SetValue(selected_index);
  }

  void OnModified(unsigned value) noexcept override
  {
    if (value < choices.size())
      on_selected(choices[value]);
  }

  void Step(int delta) noexcept override
  {
    on_step(delta);
  }
};

class LevelRowHandler final : public SelectorButtonsWidget::Handler {
  std::function<void(unsigned)> on_selected;
  std::function<void(int)> on_step;

public:
  LevelRowHandler(std::function<void(int)> _on_step,
                  std::function<void(unsigned)> _on_selected) noexcept
    :on_selected(std::move(_on_selected)),
     on_step(std::move(_on_step)) {}

  const char *GetLabel() const noexcept override
  {
    return _("Level");
  }


  void FillChoices(DataFieldEnum &field) noexcept override
  {
    field.ClearChoices();

    for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
      StaticString<32> label;
      label.Format("%u hPa (%d m)",
                   EDL::ISOBARS[i] / 100,
                   EDL::GetAltitudeForIsobar(EDL::ISOBARS[i]));
      field.AddChoice(EDL::ISOBARS[i], label.c_str());
    }

    field.SetValue(EDL::GetIsobar());
  }

  void OnModified(unsigned value) noexcept override
  {
    on_selected(value);
  }

  void Step(int delta) noexcept override
  {
    on_step(delta);
  }
};

class EdlControlsWidget final : public RowFormWidget {
  std::vector<EDL::CachedDay> cached_days;
  const bool dialog_mode;
  EDLAlwaysShowWidget *show_on_map_widget = nullptr;
  SelectorButtonsWidget *forecast_hour_widget = nullptr;
  SelectorButtonsWidget *level_widget = nullptr;
  WndProperty *cached_day_control = nullptr;
  Button *precache_day_button = nullptr;
  Button *clean_other_days_button = nullptr;

public:
  explicit EdlControlsWidget(bool _dialog_mode) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dialog_mode(_dialog_mode) {}

  void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

private:
  static StaticString<40> FormatDayLabel(const EDL::CachedDay &day) noexcept;
  void FillForecastHourWidget() noexcept;
  void FillLevelWidget() noexcept;
  void FillCachedDayControl() noexcept;
  void UpdateControls() noexcept;
  void UpdateOverlay();
  void StepForecastHour(int delta) noexcept;
  void SelectForecastHour(BrokenDateTime forecast) noexcept;
  void StepLevel(int delta) noexcept;
  void SelectLevel(unsigned isobar) noexcept;
  void PrecacheDay();
  void CleanOtherDays();
};


StaticString<40>
EdlControlsWidget::FormatDayLabel(const EDL::CachedDay &day) noexcept
{
  StaticString<40> label;
  label.Format("%04u-%02u-%02u (%s, %u)",
               day.day.year, day.day.month, day.day.day,
               day.IsComplete() ? _("Complete") : _("Partial"),
               day.file_count);
  return label;
}

void
EdlControlsWidget::FillForecastHourWidget() noexcept
{
  EDL::EnsureInitialised();
  if (forecast_hour_widget != nullptr)
    forecast_hour_widget->RefreshChoices();
}

void
EdlControlsWidget::FillLevelWidget() noexcept
{
  if (level_widget != nullptr)
    level_widget->RefreshChoices();
}

void
EdlControlsWidget::FillCachedDayControl() noexcept
{
  if (cached_day_control == nullptr)
    return;

  auto &df = (DataFieldEnum &)*cached_day_control->GetDataField();
  df.ClearChoices();

  cached_days = EDL::ListDownloadedDays();
  if (cached_days.empty()) {
    df.AddChoice(-1, _("None"));
    df.SetValue(-1);
    cached_day_control->SetEnabled(false);
    if (clean_other_days_button != nullptr)
      clean_other_days_button->SetEnabled(false);
    cached_day_control->RefreshDisplay();
    return;
  }

  cached_day_control->SetEnabled(true);
  if (clean_other_days_button != nullptr)
    clean_other_days_button->SetEnabled(true);

  unsigned selected_index = 0;
  const auto current_day = EDL::GetForecastTime().AtMidnight();
  for (unsigned i = 0; i < cached_days.size(); ++i) {
    const auto label = FormatDayLabel(cached_days[i]);
    df.AddChoice(i, label.c_str());
    if (cached_days[i].day == current_day)
      selected_index = i;
  }

  df.SetValue(selected_index);
  cached_day_control->RefreshDisplay();
}

void
EdlControlsWidget::UpdateControls() noexcept
{
  FillForecastHourWidget();
  FillLevelWidget();
  FillCachedDayControl();
}

void
EdlControlsWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{

  WindowStyle style;
  style.Hide();
  style.ControlParent();

  auto window = std::make_unique<SolidContainerWindow>();
  window->Create(parent, rc, UIGlobals::GetDialogLook().background_color, style);
  SetWindow(std::move(window));
}

void
EdlControlsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  EDL::EnsureInitialised();

  if (dialog_mode) {
    auto widget = std::make_unique<EDLAlwaysShowWidget>(
      [this]{ UpdateOverlay(); });
    show_on_map_widget = widget.get();
    Add(std::move(widget));
  }

  auto forecast_widget = std::make_unique<SelectorButtonsWidget>(
    std::make_unique<ForecastRowHandler>(
      [this](int delta){ StepForecastHour(delta); },
      [this](BrokenDateTime forecast){ SelectForecastHour(forecast); }));
  forecast_hour_widget = forecast_widget.get();
  Add(std::move(forecast_widget));

  auto level_selector_widget = std::make_unique<SelectorButtonsWidget>(
    std::make_unique<LevelRowHandler>(
      [this](int delta){ StepLevel(delta); },
      [this](unsigned isobar){ SelectLevel(isobar); }));
  level_widget = level_selector_widget.get();
  Add(std::move(level_selector_widget));

  if (dialog_mode) {
    cached_day_control = AddEnum(_("Cached day"), nullptr);
    cached_day_control->SetReadOnly(true);

#ifdef HAVE_HTTP
    precache_day_button = AddButton(_("Precache day"), [this]{ PrecacheDay(); });
#endif
    clean_other_days_button = AddButton(_("Clean other days"), [this]{ CleanOtherDays(); });
  }

  const bool enabled = EDL::OverlayEnabled();
  if (precache_day_button != nullptr)
    precache_day_button->SetEnabled(enabled);

}

void
EdlControlsWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  const auto &layout = PageActions::GetCurrentLayout();
  /* Dedicated EDL pages reset to "now/current altitude" on entry,
     while the weather-dialog tab reuses the last shared selection. */
  const bool dedicated_page = !dialog_mode && layout.main == PageLayout::Main::EDL_MAP;
  const bool reset_dedicated_page = dedicated_page && EDL::EnterDedicatedPage();

  if (reset_dedicated_page) {
    EDL::ResetForDedicatedPage();
  } else
    EDL::EnsureInitialised();

  if (show_on_map_widget != nullptr)
    show_on_map_widget->RefreshState();

  UpdateControls();

#if defined(HAVE_HTTP) && defined(ENABLE_OPENGL)
  if (reset_dedicated_page)
    UpdateOverlay();
  else if (!EDL::OverlayVisible() &&
           (dedicated_page || EDL::ShouldShowOnMainMap()))
    UpdateOverlay();
#endif
}

void
EdlControlsWidget::Unprepare() noexcept
{
  forecast_hour_widget = nullptr;
  level_widget = nullptr;
  show_on_map_widget = nullptr;
  cached_day_control = nullptr;
  precache_day_button = nullptr;
  clean_other_days_button = nullptr;
  cached_days.clear();
  RowFormWidget::Unprepare();
}

void
EdlControlsWidget::StepForecastHour(int delta) noexcept
{
  EDL::EnsureInitialised();
  CommonInterface::SetUIState().weather.edl.StepForecast(std::chrono::hours{delta});
  UpdateControls();
  UpdateOverlay();
}

void
EdlControlsWidget::SelectForecastHour(BrokenDateTime forecast) noexcept
{
  EDL::EnsureInitialised();
  CommonInterface::SetUIState().weather.edl.forecast_datetime = forecast;
  UpdateControls();
  UpdateOverlay();
}

void
EdlControlsWidget::StepLevel(int delta) noexcept
{
  EDL::EnsureInitialised();

  const unsigned current = EDL::GetIsobar();
  const auto *begin = std::begin(EDL::ISOBARS);
  const auto *end = std::end(EDL::ISOBARS);
  auto it = std::find(begin, end, current);
  unsigned index = it == end ? 0u : unsigned(std::distance(begin, it));

  if (delta < 0 && index + 1 < EDL::NUM_ISOBARS)
    ++index;
  else if (delta > 0 && index > 0)
    --index;

  CommonInterface::SetUIState().weather.edl.SelectIsobar(EDL::ISOBARS[index]);
  UpdateControls();
  UpdateOverlay();
}

void
EdlControlsWidget::SelectLevel(unsigned isobar) noexcept
{
  EDL::EnsureInitialised();
  CommonInterface::SetUIState().weather.edl.SelectIsobar(isobar);
  UpdateControls();
  UpdateOverlay();
}

void
EdlControlsWidget::UpdateOverlay()
{
#if !defined(HAVE_HTTP)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  if (!EDL::OverlayEnabled()) {
    UpdateControls();
    return;
  }

  try {
    PluggableOperationEnvironment env;
    EDL::SetLoadingStatus();
    UpdateControls();

    const EDL::TileRequest request(EDL::GetForecastTime(), EDL::GetIsobar());
    auto path = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                     UIGlobals::GetDialogLook(),
                                     _("Download"),
                                     request.EnsureDownloaded(*Net::curl, env),
                                     &env);
    if (!path) {
      EDL::SetIdleStatus();
      UpdateControls();
      return;
    }

    if (!dialog_mode || EDL::ShouldShowOnMainMap() ||
        PageActions::GetCurrentLayout().main == PageLayout::Main::EDL_MAP)
      EDL::ApplyOverlay(*path);
    else
      EDL::SetIdleStatus();
  } catch (...) {
    EDL::SetErrorStatus();
    ShowError(std::current_exception(), _("Weather"));
  }

  UpdateControls();
#endif
}

void
EdlControlsWidget::PrecacheDay()
{
#if !defined(HAVE_HTTP)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  if (!EDL::OverlayEnabled())
    return;

  try {
    PluggableOperationEnvironment env;
    const auto count = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                            UIGlobals::GetDialogLook(),
                                            _("Precache day"),
                                            EDL::EnsureDayDownloaded(EDL::GetForecastTime(),
                                                                     *Net::curl, env),
                                            &env);
    if (!count)
      return;

    UpdateControls();

    StaticString<64> message;
    message.Format(_("Cached %u files for the selected UTC day."), *count);
    ShowMessageBox(message, _("Weather"), MB_OK);
  } catch (...) {
    ShowError(std::current_exception(), _("Weather"));
  }
#endif
}

void
EdlControlsWidget::CleanOtherDays()
{
  if (cached_days.empty())
    return;

  const auto selected_index =
    ((const DataFieldEnum &)*cached_day_control->GetDataField()).GetValue();
  if ((unsigned)selected_index >= cached_days.size())
    return;

  StaticString<96> message;
  message.Format(_("Keep only %04u-%02u-%02u and delete the other cached days?"),
                 cached_days[selected_index].day.year,
                 cached_days[selected_index].day.month,
                 cached_days[selected_index].day.day);
  if (ShowMessageBox(message, _("Weather"), MB_YESNO) != IDYES)
    return;

  const unsigned deleted = EDL::DeleteOtherDownloadedDays(cached_days[selected_index].day);
  UpdateControls();

  StaticString<64> result;
  result.Format(_("Deleted %u cached files."), deleted);
  ShowMessageBox(result, _("Weather"), MB_OK);
}

std::unique_ptr<Widget>
CreateEdlControlsOverlayWidget()
{
  return std::make_unique<EdlControlsWidget>(true);
}

std::unique_ptr<Widget>
CreateEdlControlsBottomWidget()
{
  return std::make_unique<EdlControlsWidget>(false);
}
