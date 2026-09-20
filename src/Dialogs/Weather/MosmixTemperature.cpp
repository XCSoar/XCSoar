// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MosmixTemperature.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Weather/MOSMIX/AutoUpdate.hpp"
#include "Weather/MOSMIX/Download.hpp"
#include "net/http/Features.hpp"
#include "net/http/Init.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "time/BrokenDateTime.hpp"

std::optional<Temperature>
MaybeFetchForecastTemperature() noexcept
try {
#ifdef HAVE_HTTP
  const auto now = BrokenDateTime::NowUTC();
  if (!now.IsPlausible())
    return std::nullopt;

  const BrokenDate today = now;
  if (!MOSMIX::ShouldFetchToday(today))
    return std::nullopt;

  const auto &basic = CommonInterface::Basic();
  if (!basic.location_available)
    /* the station is picked by distance, so there is nothing to pick
       it from; try again once there is a fix */
    return std::nullopt;

  if (Net::curl == nullptr)
    return std::nullopt;

  PluggableOperationEnvironment env;
  const auto result = ShowCoFunctionDialog(
    UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
    _("Forecast temperature"),
    MOSMIX::CoFetchForecastMaximum(*Net::curl, basic.location, today, env),
    &env);

  if (!result)
    /* cancelled; leave the day open so the next try is not blocked */
    return std::nullopt;

  /* Remember the day even when the forecast carried no value: asking
     again on the same day would only repeat the same answer, and the
     pilot did not ask for this in the first place. */
  MOSMIX::RememberFetch(today);

  return *result;
#else
  return std::nullopt;
#endif
} catch (...) {
  /* Logged, not shown.  The usual reason is that there is no network,
     and a dialog about it on the first opening of every day would be
     a nuisance about something the pilot did not ask for.  The flight
     setup opens with the value it already had. */
  LogError(std::current_exception(), "MOSMIX forecast temperature");
  return std::nullopt;
}
