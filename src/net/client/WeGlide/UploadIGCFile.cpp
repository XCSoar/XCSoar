// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UploadIGCFile.hpp"
#include "UploadFlight.hpp"
#include "Settings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "co/InvokeTask.hxx"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/CoDialog.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "json/ParserOutputStream.hxx"
#include "Language/Language.hpp"
#include "lib/fmt/tchar.hxx"
#include "net/http/Init.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "util/ConvertString.hpp"

#include <cinttypes>

// Wrapper for getting converted string values of a json string
static const UTF8ToWideConverter 
GetJsonString(boost::json::value json_value, std::string_view key)
{
  return UTF8ToWideConverter(json_value.at(key).get_string().c_str());
}

namespace WeGlide {

struct User {
  uint32_t id;
  BrokenDate birthdate;
  StaticString<0x80> name;
};

struct Aircraft {
  uint32_t id;
  StaticString<0x40> name;
  StaticString<4> kind;  // 'MG' - motor aircraft,...
  StaticString<10> sc_class;
};

struct FlightData {
  uint64_t flight_id = 0;
  User user;
  Aircraft aircraft;
  StaticString<0x40> scoring_date;
  StaticString<0x40> registration;
  StaticString<0x40> competition_id;
};

static FlightData
UploadJsonInterpreter(const boost::json::value &json)
{
  FlightData flight_data;
  // flight is the 1st flight object in this array ('at(0)')
  auto flight = json.as_array().at(0);
  flight_data.scoring_date = GetJsonString(flight, "scoring_date").c_str();
  flight_data.flight_id = flight.at("id").to_number<int64_t>();
  flight_data.registration = GetJsonString(flight, "registration").c_str();
  flight_data.competition_id = GetJsonString(flight, "competition_id").c_str();

  auto user = flight.at("user").as_object();
  flight_data.user.id = user.at("id").to_number<uint32_t>();
  flight_data.user.name = GetJsonString(user, "name").c_str();

  auto aircraft = flight.at("aircraft").as_object();
  flight_data.aircraft.id = aircraft.at("id").to_number<uint32_t>();
  flight_data.aircraft.name = GetJsonString(aircraft, "name").c_str();
  flight_data.aircraft.kind = GetJsonString(aircraft, "kind").c_str();
  flight_data.aircraft.sc_class = GetJsonString(aircraft, "sc_class").c_str();

  return flight_data;
}

// UploadSuccessDialog is only a preliminary DialogBox to show the 
// result of this upload
static void
UploadSuccessDialog(const FlightData &flight_data) noexcept
{
  // TODO: Create a real Dialog with fields in 'src/Dialogs/Cloud/weglide'!
  // With this Dialog insert the possibilty to update/patch the flight
  // f.e. copilot in double seater, scoring class, short comment and so on
  const auto display_string = fmt::format(_T("{}: {}\n{}: {}\n{}: {} ({})\n"
                                             "{}: {} ({})\n{}: {}, {}: {}"),
    _T("Flight ID"), flight_data.flight_id,
    _("Date"), flight_data.scoring_date.c_str(),
    _("Username"), flight_data.user.name.c_str(), flight_data.user.id,
    _("Plane"), flight_data.aircraft.name.c_str(), flight_data.aircraft.id,
    _("Registration"), flight_data.registration.c_str(),
    _("Comp. ID"), flight_data.competition_id.c_str());

  ShowMessageBox(display_string.c_str(), _("WeGlide Upload"), MB_OK);
}

struct CoInstance {
  boost::json::value value;
  Co::InvokeTask
  UpdateTask(Path igc_path, const WeGlideSettings &settings,
    uint_least32_t glider_id, ProgressListener &progress)
  {
    value = co_await UploadFlight(*Net::curl, settings, glider_id,
      igc_path, progress);
  }
};

static FlightData
UploadFile(Path igc_path)
{
  WeGlideSettings settings = CommonInterface::GetComputerSettings().weglide;
  uint32_t glider_id = CommonInterface::GetComputerSettings().plane
    .weglide_glider_type;

  PluggableOperationEnvironment env;
  CoInstance instance;
  if (ShowCoDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                   _("Upload Flight"), instance.UpdateTask(igc_path, settings,
                                                           glider_id, env), &env) == false) {
    return {};
  }

  // read the important data from json in a structure
  return UploadJsonInterpreter(instance.value);
}

bool
UploadIGCFile(Path igc_path) noexcept
try {
  auto flight_data = UploadFile(igc_path);
  if (flight_data.flight_id == 0)
    /* cancelled by the user */
    return false;

  UploadSuccessDialog(flight_data);
  return true;
} catch (...) {
  ShowError(std::current_exception(), _("Error"));
  return false;
}

} // namespace WeGlide
