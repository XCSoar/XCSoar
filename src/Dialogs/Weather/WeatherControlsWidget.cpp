// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherControlsWidget.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
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
#include "Weather/EDL/Download.hpp"
#endif
#include "Weather/EDL/Manager.hpp"
#include "Weather/EDL/Request.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/dim/Size.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "util/StaticString.hxx"

#include <array>
#include <chrono>
#include <memory>
#include <vector>

class EDLAlwaysShowWidget final : public WindowWidget {
public:
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
                     [](bool enabled){
                       EDL::SetShowOnMainMap(enabled);
                       if (!enabled)
                         EDL::RefreshOverlayVisibility();
                     });
    SetWindow(std::move(checkbox));
  }

  void Show(const PixelRect &rc) noexcept override
  {
    auto &checkbox = (CheckBoxControl &)GetWindow();
    checkbox.SetState(EDL::ShouldShowOnMainMap());
    WindowWidget::Show(rc);
  }

  void RefreshState() noexcept
  {
    auto &checkbox = (CheckBoxControl &)GetWindow();
    checkbox.SetState(EDL::ShouldShowOnMainMap());
  }
};

class WeatherControlsWidget final : public RowFormWidget {
  /* The bottom widget stays compact by exposing time/level as generic
     selectors instead of a row per action. */
  static constexpr unsigned FORECAST_CHOICES = 24;

  std::array<BrokenDateTime, FORECAST_CHOICES> forecast_choices{};
  std::vector<EDL::CachedDay> cached_days;
  const bool dialog_mode;
  EDLAlwaysShowWidget *show_on_map_widget = nullptr;
  WndProperty *forecast_control = nullptr;
  WndProperty *level_control = nullptr;
  WndProperty *cached_day_control = nullptr;
  Button *precache_day_button = nullptr;
  Button *clean_other_days_button = nullptr;

public:
  explicit WeatherControlsWidget(bool _dialog_mode) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dialog_mode(_dialog_mode) {}

  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

private:
  static StaticString<40> FormatDayLabel(const EDL::CachedDay &day) noexcept;
  void FillForecastControl() noexcept;
  void FillLevelControl() noexcept;
  void FillCachedDayControl() noexcept;
  void UpdateControls() noexcept;
  void UpdateOverlay();
  void PrecacheDay();
  void CleanOtherDays();
};

PixelSize
WeatherControlsWidget::GetMinimumSize() const noexcept
{
  const unsigned caption_width =
    UIGlobals::GetDialogLook().text_font.TextSize("Forecast").width;
  const unsigned value_width =
    UIGlobals::GetDialogLook().text_font.TextSize("900 hPa (9999 m)").width;
  const unsigned checkbox_width =
    CheckBoxControl::GetMinimumWidth(UIGlobals::GetDialogLook(),
                                     Layout::GetMinimumControlHeight(),
                                     _("Always show EDL on map"));
  const unsigned row_height = Layout::GetMinimumControlHeight();
  const unsigned rows = dialog_mode ? 6u : 2u;

  return {std::max(caption_width + value_width, checkbox_width), row_height * rows};
}

PixelSize
WeatherControlsWidget::GetMaximumSize() const noexcept
{
  const auto minimum = GetMinimumSize();
  const unsigned rows = dialog_mode ? 6u : 2u;
  return {minimum.width, Layout::GetMaximumControlHeight() * rows};
}

void
WeatherControlsWidget::Initialise(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  assert(!IsDefined());

  WindowStyle style;
  style.Hide();
  style.ControlParent();

  auto window = std::make_unique<SolidContainerWindow>();
  window->Create(parent, rc, UIGlobals::GetDialogLook().background_color, style);
  SetWindow(std::move(window));
}

StaticString<40>
WeatherControlsWidget::FormatDayLabel(const EDL::CachedDay &day) noexcept
{
  StaticString<40> label;
  label.Format("%04u-%02u-%02u (%s, %u)",
               day.day.year, day.day.month, day.day.day,
               day.IsComplete() ? _("Complete") : _("Partial"),
               day.file_count);
  return label;
}

void
WeatherControlsWidget::FillForecastControl() noexcept
{
  /* The widget may be prepared before the EDL state has been touched in
     this session.  Normalise it before doing BrokenDateTime arithmetic. */
  EDL::EnsureInitialised();

  auto &df = (DataFieldEnum &)*forecast_control->GetDataField();
  df.ClearChoices();

  const auto selected_time = EDL::GetForecastTime();
  /* Offer a small centred window around the current hour instead of
     forcing a very tall button list in the bottom panel. */
  const auto base_time = selected_time + std::chrono::hours{-11};

  unsigned selected_index = 0;
  for (unsigned i = 0; i < FORECAST_CHOICES; ++i) {
    forecast_choices[i] = base_time + std::chrono::hours{i};
    const auto local_time = EDL::ToLocalForecastTime(forecast_choices[i]);

    StaticString<32> label;
    label.Format("%02u:00", unsigned(local_time.hour));
    df.AddChoice(i, label.c_str());

    if (forecast_choices[i] == selected_time)
      selected_index = i;
  }

  df.SetValue(selected_index);
  forecast_control->RefreshDisplay();
}

void
WeatherControlsWidget::FillLevelControl() noexcept
{
  auto &df = (DataFieldEnum &)*level_control->GetDataField();
  df.ClearChoices();

  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
    StaticString<32> label;
    label.Format("%u hPa (%d m)",
                 EDL::ISOBARS[i] / 100,
                 EDL::GetAltitudeForIsobar(EDL::ISOBARS[i]));
    df.AddChoice(EDL::ISOBARS[i], label.c_str());
  }

  df.SetValue(EDL::GetIsobar());
  level_control->RefreshDisplay();
}

void
WeatherControlsWidget::FillCachedDayControl() noexcept
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
WeatherControlsWidget::UpdateControls() noexcept
{
  FillForecastControl();
  FillLevelControl();
  FillCachedDayControl();
}

void
WeatherControlsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  EDL::EnsureInitialised();

  if (dialog_mode) {
    auto widget = std::make_unique<EDLAlwaysShowWidget>();
    show_on_map_widget = widget.get();
    Add(std::move(widget));
  }

  forecast_control = AddEnum(_("Forecast"), nullptr);
  forecast_control->GetDataField()->SetOnModified([this]{
    const auto index =
      ((const DataFieldEnum &)*forecast_control->GetDataField()).GetValue();
    if (index < forecast_choices.size())
      CommonInterface::SetUIState().weather.forecast_datetime = forecast_choices[index];

    FillForecastControl();
    UpdateControls();
    UpdateOverlay();
  });

  level_control = AddEnum(_("Level"), nullptr);
  level_control->GetDataField()->SetOnModified([this]{
    EDL::SelectIsobar(((const DataFieldEnum &)*level_control->GetDataField()).GetValue());
    FillLevelControl();
    UpdateControls();
    UpdateOverlay();
  });

  if (dialog_mode) {
    cached_day_control = AddEnum(_("Cached day"), nullptr);
    cached_day_control->SetReadOnly(true);

#ifdef HAVE_HTTP
    precache_day_button = AddButton(_("Precache day"), [this]{ PrecacheDay(); });
#endif
    clean_other_days_button = AddButton(_("Clean other days"), [this]{ CleanOtherDays(); });
  }

  const bool enabled = EDL::OverlayEnabled();
  forecast_control->SetEnabled(enabled);
  level_control->SetEnabled(enabled);
  if (precache_day_button != nullptr)
    precache_day_button->SetEnabled(enabled);

  UpdateControls();
}

void
WeatherControlsWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  const auto &layout = PageActions::GetCurrentLayout();
  /* Dedicated EDL pages reset to "now/current altitude" on entry,
     while the weather-dialog tab reuses the last shared selection. */
  const bool dedicated_page = !dialog_mode && layout.main == PageLayout::Main::EDL_MAP;

  if (dedicated_page)
    EDL::ResetForDedicatedPage();
  else
    EDL::EnsureInitialised();

  if (show_on_map_widget != nullptr)
    show_on_map_widget->RefreshState();

  UpdateControls();

#if defined(HAVE_HTTP) && defined(ENABLE_OPENGL)
  if (dedicated_page || !EDL::OverlayVisible())
    UpdateOverlay();
#endif
}

void
WeatherControlsWidget::Unprepare() noexcept
{
  forecast_control = nullptr;
  level_control = nullptr;
  show_on_map_widget = nullptr;
  cached_day_control = nullptr;
  precache_day_button = nullptr;
  clean_other_days_button = nullptr;
  cached_days.clear();
  RowFormWidget::Unprepare();
}

void
WeatherControlsWidget::UpdateOverlay()
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

    auto path = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                     UIGlobals::GetDialogLook(),
                                     _("Download"),
                                     EDL::EnsureDownloaded(EDL::GetForecastTime(),
                                                           EDL::GetIsobar(),
                                                           *Net::curl, env),
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
WeatherControlsWidget::PrecacheDay()
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
WeatherControlsWidget::CleanOtherDays()
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
CreateWeatherControlsOverlayWidget() noexcept
{
  return std::make_unique<WeatherControlsWidget>(true);
}

std::unique_ptr<Widget>
CreateWeatherControlsBottomWidget() noexcept
{
  return std::make_unique<WeatherControlsWidget>(false);
}
