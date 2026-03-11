// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherPageWidget.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "PageActions.hpp"
#include "UIGlobals.hpp"
#include "net/http/Features.hpp"
#ifdef HAVE_HTTP
#include "net/http/Init.hpp"
#include "Weather/EDL/Download.hpp"
#include "Weather/EDL/MbTilesOverlay.hpp"
#endif
#include "Weather/EDL/LevelResolver.hpp"
#include "Weather/EDL/Request.hpp"
#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"

#include <algorithm>

class WeatherPageWidget final : public RowFormWidget {
  enum Rows {
    SUMMARY,
  };

  Button *time_minus_button = nullptr;
  Button *time_plus_button = nullptr;
  Button *altitude_minus_button = nullptr;
  Button *altitude_plus_button = nullptr;
  Button *refresh_button = nullptr;
  Button *close_button = nullptr;

public:
  WeatherPageWidget() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

private:
  void InitialiseState() noexcept;
  void UpdateSummary() noexcept;
  void UpdateOverlay();
  void StepForecast(std::chrono::hours delta);
  void StepAltitude(int delta) noexcept;
  void ClosePage();
};

void
WeatherPageWidget::InitialiseState() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  if (!state.forecast_datetime.IsPlausible()) {
    const auto now = BrokenDateTime::NowUTC();
    state.forecast_datetime = BrokenDateTime(now.year, now.month, now.day,
                                             now.hour, 0, 0);
  }

  const auto resolved = EDL::ResolveLevel(CommonInterface::GetComputerSettings().pressure,
                                          CommonInterface::GetComputerSettings().pressure_available,
                                          state.edl_altitude);
  state.edl_isobar = resolved.isobar;

#ifdef HAVE_HTTP
  if (CommonInterface::GetComputerSettings().weather.enable_edl &&
      state.edl_status == WeatherUIState::EDLStatus::DISABLED)
    state.edl_status = WeatherUIState::EDLStatus::IDLE;
#endif
}

void
WeatherPageWidget::UpdateSummary() noexcept
{
  const auto &state = CommonInterface::GetUIState().weather;
  StaticString<256> summary;
  StaticString<32> forecast;
  EDL::FormatForecastParameter(forecast.buffer(), state.forecast_datetime);

  const char *status = nullptr;
  switch (state.edl_status) {
  case WeatherUIState::EDLStatus::DISABLED:
    status = _("Disabled");
    break;
  case WeatherUIState::EDLStatus::IDLE:
    status = _("Idle");
    break;
  case WeatherUIState::EDLStatus::LOADING:
    status = _("Loading");
    break;
  case WeatherUIState::EDLStatus::READY:
    status = _("Ready");
    break;
  case WeatherUIState::EDLStatus::ERROR:
    status = _("Error");
    break;
  }

  summary.Format("EDL\n%s\n%d m -> %u hPa\n%s",
                 forecast.c_str(),
                 state.edl_altitude,
                 state.edl_isobar / 100,
                 status);
  SetMultiLineText(SUMMARY, summary.c_str());
}

void
WeatherPageWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  InitialiseState();

  AddMultiLine("");
  time_minus_button = AddButton(_("Time -"), [this]{ StepForecast(-std::chrono::hours{1}); });
  time_plus_button = AddButton(_("Time +"), [this]{ StepForecast(std::chrono::hours{1}); });
  altitude_minus_button = AddButton(_("Altitude -"), [this]{ StepAltitude(-500); });
  altitude_plus_button = AddButton(_("Altitude +"), [this]{ StepAltitude(500); });
  refresh_button = AddButton(_("Refresh"), [this]{ UpdateOverlay(); });
  close_button = AddButton(_("Close"), [this]{ ClosePage(); });

#ifdef HAVE_HTTP
  const bool enabled = CommonInterface::GetComputerSettings().weather.enable_edl;
#else
  const bool enabled = false;
#endif
  time_minus_button->SetEnabled(enabled);
  time_plus_button->SetEnabled(enabled);
  altitude_minus_button->SetEnabled(enabled);
  altitude_plus_button->SetEnabled(enabled);
  refresh_button->SetEnabled(enabled);
  UpdateSummary();
}

void
WeatherPageWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  UpdateSummary();

#if defined(HAVE_HTTP) && defined(ENABLE_OPENGL)
  const auto &state = CommonInterface::GetUIState().weather;
  if (state.edl_status == WeatherUIState::EDLStatus::IDLE) {
    const auto *map = UIGlobals::GetMap();
    if (map == nullptr || map->GetOverlay() == nullptr)
      UpdateOverlay();
  }
#endif
}

void
WeatherPageWidget::Unprepare() noexcept
{
  if (auto *map = UIGlobals::GetMap())
    map->SetOverlay(nullptr);
  RowFormWidget::Unprepare();
}

void
WeatherPageWidget::StepForecast(std::chrono::hours delta)
{
  auto &state = CommonInterface::SetUIState().weather;
  state.forecast_datetime = state.forecast_datetime + delta;
  UpdateSummary();
  UpdateOverlay();
}

void
WeatherPageWidget::StepAltitude(int delta) noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  state.edl_altitude = std::max(0, state.edl_altitude + delta);

  const auto resolved = EDL::ResolveLevel(CommonInterface::GetComputerSettings().pressure,
                                          CommonInterface::GetComputerSettings().pressure_available,
                                          state.edl_altitude);
  state.edl_isobar = resolved.isobar;
  UpdateSummary();
  UpdateOverlay();
}

void
WeatherPageWidget::UpdateOverlay()
{
#if !defined(HAVE_HTTP)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
  return;
#else
  auto &state = CommonInterface::SetUIState().weather;
  if (!CommonInterface::GetComputerSettings().weather.enable_edl) {
    state.edl_status = WeatherUIState::EDLStatus::DISABLED;
    UpdateSummary();
    return;
  }

  state.edl_status = WeatherUIState::EDLStatus::LOADING;
  UpdateSummary();

  try {
    PluggableOperationEnvironment env;
    auto path = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                     UIGlobals::GetDialogLook(),
                                     _("Download"),
                                     EDL::EnsureDownloaded(state.forecast_datetime,
                                                           state.edl_isobar,
                                                           *Net::curl, env),
                                     &env);
    if (!path) {
      state.edl_status = WeatherUIState::EDLStatus::IDLE;
      UpdateSummary();
      return;
    }

    auto label = EDL::BuildCacheFileName(state.forecast_datetime, state.edl_isobar);
    auto overlay = std::make_unique<EDL::MbTilesOverlay>(*path, label.c_str());

    if (auto *map = UIGlobals::GetMap()) {
      map->SetOverlay(std::move(overlay));
      map->QuickRedraw();
    }

    state.edl_enabled = true;
    state.edl_status = WeatherUIState::EDLStatus::READY;
  } catch (...) {
    state.edl_status = WeatherUIState::EDLStatus::ERROR;
    UpdateSummary();
    ShowError(std::current_exception(), _("Weather"));
    return;
  }

  UpdateSummary();
#endif
}

void
WeatherPageWidget::ClosePage()
{
  PageActions::Restore();
}

std::unique_ptr<Widget>
CreateWeatherPageWidget() noexcept
{
  return std::make_unique<WeatherPageWidget>();
}
