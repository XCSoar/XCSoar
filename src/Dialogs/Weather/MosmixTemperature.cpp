// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MosmixTemperature.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Weather/MOSMIX/AutoUpdate.hpp"
#include "Weather/MOSMIX/Download.hpp"
#include "net/http/Features.hpp"
#include "net/http/Init.hpp"
#include "time/BrokenDateTime.hpp"
#include "UIGlobals.hpp"
#include "io/async/AsioThread.hpp"
#include "io/async/GlobalAsioThread.hpp"

Co::InvokeTask
ForecastTemperatureFetcher::Run()
{
#ifdef HAVE_HTTP
  const BrokenDate date{uint16_t(year), uint8_t(month), uint8_t(day)};
  NullOperationEnvironment env;
  result = co_await MOSMIX::CoFetchForecastMaximum(*Net::curl, location,
                                                   date, env);
#else
  co_return;
#endif
}

void
ForecastTemperatureFetcher::OnCompletion(std::exception_ptr _error) noexcept
{
  error = std::move(_error);
  complete_notify.SendNotification();
}

void
ForecastTemperatureFetcher::OnCompleteNotify() noexcept
{
  if (error) {
    /* Logged, not shown.  The usual reason is that there is no
       network, and a dialog about it on the first opening of every
       day -- about something the pilot did not ask for -- would be a
       nuisance in its own right.  The field keeps the value it had. */
    LogError(std::move(error), "MOSMIX forecast temperature");
    error = {};
    return;
  }

  const BrokenDate date{uint16_t(year), uint8_t(month), uint8_t(day)};

  /* Remembered even when the forecast carried no value: asking again
     the same day would only repeat the answer. */
  MOSMIX::RememberFetch(date, result);

  if (result.has_value() && on_result)
    on_result(*result);
}

void
ForecastTemperatureFetcher::Start(std::function<void(Temperature)>
                                  &&callback) noexcept
{
#ifdef HAVE_HTTP
  if (!CommonInterface::GetComputerSettings()
       .weather.mosmix_forecast_temperature)
    /* off unless the pilot asked for it: this contacts the DWD by
       itself, which is not something to do unannounced */
    return;

  if (Net::curl == nullptr)
    return;

  const auto now = BrokenDateTime::NowUTC();
  if (!now.IsPlausible())
    return;

  const BrokenDate today = now;

  /* already answered today: show that, and stay off the network */
  if (const auto stored = MOSMIX::GetStoredForecast(today)) {
    if (callback)
      callback(*stored);
    return;
  }

  if (!MOSMIX::ShouldFetchToday(today))
    return;

  if (!CommonInterface::Basic().location_available)
    /* the station is picked by distance, so there is nothing to pick
       it from; the next opening with a fix will try again */
    return;

  year = today.year;
  month = today.month;
  day = today.day;
  location = CommonInterface::Basic().location;
  on_result = std::move(callback);

  task.emplace(asio_thread->GetEventLoop());
  task->Start(Run(), BIND_THIS_METHOD(OnCompletion));
#else
  (void)callback;
#endif
}
