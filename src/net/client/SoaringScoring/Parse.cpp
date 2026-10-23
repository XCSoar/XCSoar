// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Parse.hpp"
#include "Auth.hpp"

#include <boost/json.hpp>

#include <cmath>
#include <stdexcept>

using std::string_view_literals::operator""sv;

namespace SoaringScoring {
namespace {

std::string
String(const boost::json::object &object, std::string_view key)
{
  const auto *value = object.if_contains(key);
  if (value == nullptr || !value->is_string())
    return {};
  return std::string{value->as_string()};
}

bool
Flag(const boost::json::object &object, std::string_view key) noexcept
{
  const auto *value = object.if_contains(key);
  return value != nullptr && value->is_bool() && value->as_bool();
}

bool
Number(const boost::json::object &object, std::string_view key,
       double &out) noexcept
{
  const auto *value = object.if_contains(key);
  if (value == nullptr || !value->is_number())
    return false;
  out = value->to_number<double>();
  return true;
}

boost::json::value
ParseJson(std::string_view json)
{
  try {
    return boost::json::parse(json);
  } catch (const std::exception &) {
    throw std::runtime_error("Invalid SoaringScoring response");
  }
}

const boost::json::object &
Object(const boost::json::value &value)
{
  if (!value.is_object())
    throw std::runtime_error("Invalid SoaringScoring response");
  return value.as_object();
}

ContestWhen
ParseWhen(std::string_view text) noexcept
{
  if (text == "current"sv)
    return ContestWhen::CURRENT;
  if (text == "future"sv)
    return ContestWhen::FUTURE;
  if (text == "past"sv)
    return ContestWhen::PAST;
  return ContestWhen::UNKNOWN;
}

Entry
ParseEntry(const boost::json::object &object)
{
  Entry entry;
  entry.class_name = String(object, "className");
  entry.upload_url = String(object, "uploadUrl");
  if (const auto *contest = object.if_contains("contest");
      contest != nullptr && contest->is_object()) {
    const auto &contest_object = contest->as_object();
    entry.contest_id = String(contest_object, "id");
    entry.contest_name = String(contest_object, "name");
    entry.when = ParseWhen(String(contest_object, "when"));
  }
  return entry;
}

TaskRow
ParseRow(const boost::json::object &object)
{
  TaskRow row;
  row.display_label = String(object, "displayLabel");
  row.class_name = String(object, "className");
  row.date = String(object, "date");
  row.official = Flag(object, "isOfficialTask");
  row.has_handicap = Number(object, "dhtHandicap", row.handicap);
  if (!Number(object, "dhtDistanceKm", row.nominal_distance_km))
    row.has_distance = false;
  else
    row.has_distance = true;

  if (const auto *files = object.if_contains("files");
      files != nullptr && files->is_object())
    row.xcsoar_tsk_url = String(files->as_object(), "xcsoarTsk");

  if (const auto *task = object.if_contains("task");
      task != nullptr && task->is_object()) {
    const auto &task_object = task->as_object();
    row.task_kind = String(task_object, "taskKind");
    if (auto label = String(task_object, "label"); !label.empty())
      row.display_label = std::move(label);
    double distance = 0;
    if (!row.has_distance &&
        Number(task_object, "nominalDistanceKm", distance)) {
      row.nominal_distance_km = distance;
      row.has_distance = true;
    }
  }

  if (row.display_label.empty())
    row.display_label = row.date;
  return row;
}

} // namespace

std::string
AbsoluteUrl(std::string_view url)
{
  if (url.starts_with("https://"sv) || url.starts_with("http://"sv))
    return std::string{url};
  std::string absolute{kOrigin};
  if (url.empty() || url.front() != '/')
    absolute.push_back('/');
  absolute.append(url);
  return absolute;
}

std::string
ErrorMessage(std::string_view json) noexcept
{
  try {
    const auto value = boost::json::parse(json);
    if (!value.is_object())
      return {};
    const auto *error = value.as_object().if_contains("error");
    if (error == nullptr)
      return {};
    if (error->is_string())
      return std::string{error->as_string()};
    if (!error->is_object())
      return {};
    auto message = String(error->as_object(), "message");
    if (!message.empty())
      return message;
    return String(error->as_object(), "code");
  } catch (...) {
    return {};
  }
}

Pilot
ParsePilot(std::string_view json)
{
  const auto value = ParseJson(json);
  const auto &root = Object(value);
  Pilot pilot;
  if (const auto *identity = root.if_contains("pilot");
      identity != nullptr && identity->is_object())
    pilot.name = String(identity->as_object(), "name");

  const auto *entries = root.if_contains("entries");
  if (entries == nullptr || !entries->is_array())
    return pilot;

  for (const auto &item : entries->as_array()) {
    if (!item.is_object())
      continue;
    auto entry = ParseEntry(item.as_object());
    if (!entry.contest_id.empty())
      pilot.entries.push_back(std::move(entry));
  }
  return pilot;
}

TaskList
ParseTaskList(std::string_view json)
{
  const auto value = ParseJson(json);
  const auto &root = Object(value);
  TaskList list;
  if (const auto *contest = root.if_contains("contest");
      contest != nullptr && contest->is_object())
    list.contest_name = String(contest->as_object(), "name");

  const auto *tasks = root.if_contains("tasks");
  if (tasks == nullptr || !tasks->is_array())
    return list;

  for (const auto &item : tasks->as_array()) {
    if (!item.is_object())
      continue;
    auto row = ParseRow(item.as_object());
    if (!row.xcsoar_tsk_url.empty())
      list.tasks.push_back(std::move(row));
  }
  return list;
}

UploadReport
ParseUploadReport(std::string_view json)
{
  const auto value = ParseJson(json);
  const auto &root = Object(value);
  const auto *upload = root.if_contains("upload");
  if (upload == nullptr || !upload->is_object())
    throw std::runtime_error("Invalid SoaringScoring response");

  const auto &object = upload->as_object();
  UploadReport report;
  report.validation_ok = Flag(object, "validationOk");
  const auto *issues = object.if_contains("validationIssues");
  if (issues == nullptr || !issues->is_array())
    return report;

  for (const auto &item : issues->as_array()) {
    if (item.is_string())
      report.issues.emplace_back(item.as_string());
    else if (item.is_object()) {
      auto message = String(item.as_object(), "message");
      if (!message.empty())
        report.issues.push_back(std::move(message));
    }
  }
  return report;
}

std::vector<Entry>
ActiveEntries(const std::vector<Entry> &entries)
{
  std::vector<Entry> current, future;
  for (const auto &entry : entries) {
    if (entry.when == ContestWhen::CURRENT)
      current.push_back(entry);
    else if (entry.when == ContestWhen::FUTURE)
      future.push_back(entry);
  }
  if (!current.empty())
    return current;
  if (!future.empty())
    return future;
  return entries;
}

bool
RowMatchesClass(const TaskRow &row, std::string_view class_name) noexcept
{
  return class_name.empty() || row.class_name == class_name;
}

unsigned
SuggestTask(std::span<const TaskRow> rows, unsigned handicap) noexcept
{
  unsigned official = 0;
  bool saw_official = false;
  for (unsigned i = 0; i < rows.size(); ++i) {
    const auto &row = rows[i];
    if (handicap != 0 && row.has_handicap &&
        static_cast<unsigned>(std::lround(row.handicap)) == handicap)
      return i;
    if (!saw_official && row.official) {
      official = i;
      saw_official = true;
    }
  }
  return official;
}

} // namespace SoaringScoring
