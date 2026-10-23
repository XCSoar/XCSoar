// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "net/client/SoaringScoring/Parse.hpp"
#include "TestUtil.hpp"

static void
TestUrls()
{
  ok1(SoaringScoring::AbsoluteUrl("/api/v1/public/me/entries") ==
      "https://soaringscoring.com/api/v1/public/me/entries");
  ok1(SoaringScoring::AbsoluteUrl("https://soaringscoring.com/device") ==
      "https://soaringscoring.com/device");
}

static constexpr char kPilot[] =
  R"({"pilot":{"name":"Jane Smith"},"entries":[)"
  R"({"contest":{"id":"future","name":"Future","when":"future"},)"
  R"("className":"Club","uploadUrl":"/api/v1/public/entries/c-future/igc"},)"
  R"({"contest":{"id":"now","name":"Nationals","when":"current"},)"
  R"("className":"Open Class",)"
  R"("uploadUrl":"/api/v1/public/entries/oa-nats/igc"}]})";

static void
TestPilot()
{
  const auto pilot = SoaringScoring::ParsePilot(kPilot);
  ok1(pilot.name == "Jane Smith");
  ok1(pilot.entries.size() == 2);

  const auto active = SoaringScoring::ActiveEntries(pilot.entries);
  ok1(active.size() == 1);
  ok1(active[0].contest_id == "now");
  ok1(active[0].class_name == "Open Class");
  ok1(active[0].when == SoaringScoring::ContestWhen::CURRENT);

  std::vector<SoaringScoring::Entry> future_only{pilot.entries[0]};
  const auto future = SoaringScoring::ActiveEntries(future_only);
  ok1(future.size() == 1);
  ok1(future[0].when == SoaringScoring::ContestWhen::FUTURE);
}

static constexpr char kTasks[] =
  R"({"contest":{"name":"Nationals"},"tasks":[)"
  R"({"displayLabel":"Day 1","className":"Open Class","date":"2025-01-12",)"
  R"("isOfficialTask":true,"files":{"xcsoarTsk":"/tsk/day1"}},)"
  R"({"displayLabel":"Day 1","className":"Club","date":"2025-01-12",)"
  R"("isOfficialTask":false,"files":{"xcsoarTsk":"/tsk/club"}},)"
  R"({"displayLabel":"Day 2","className":"Open Class","date":"2025-01-13",)"
  R"("isOfficialTask":true,"dhtHandicap":108,"dhtDistanceKm":312.5,)"
  R"("task":{"label":"Day 2 Open","taskKind":"distance_handicap",)"
  R"("nominalDistanceKm":300},"files":{"xcsoarTsk":"/tsk/dht"}}]})";

static void
TestTasks()
{
  const auto list = SoaringScoring::ParseTaskList(kTasks);
  ok1(list.contest_name == "Nationals");
  ok1(list.tasks.size() == 3);
  ok1(list.tasks[0].official);
  ok1(list.tasks[0].xcsoar_tsk_url == "/tsk/day1");
  ok1(!list.tasks[0].has_handicap);
  ok1(list.tasks[2].has_handicap);
  ok1(list.tasks[2].handicap == 108);
  ok1(list.tasks[2].has_distance);
  ok1(list.tasks[2].nominal_distance_km == 312.5);
  ok1(list.tasks[2].task_kind == "distance_handicap");
  ok1(list.tasks[2].display_label == "Day 2 Open");
  ok1(SoaringScoring::RowMatchesClass(list.tasks[0], "Open Class"));
  ok1(!SoaringScoring::RowMatchesClass(list.tasks[1], "Open Class"));

  std::vector<SoaringScoring::TaskRow> open;
  for (const auto &row : list.tasks)
    if (SoaringScoring::RowMatchesClass(row, "Open Class"))
      open.push_back(row);
  ok1(SoaringScoring::SuggestTask(open, 108) == 1);
  ok1(SoaringScoring::SuggestTask(open, 0) == 0);
}

static void
TestUploadAndError()
{
  const auto report = SoaringScoring::ParseUploadReport(
    R"({"upload":{"validationOk":false,"validationIssues":["bad fix"]}})");
  ok1(!report.validation_ok);
  ok1(report.issues.size() == 1);
  ok1(report.issues[0] == "bad fix");
  ok1(SoaringScoring::ErrorMessage(
        R"({"error":{"code":"INVALID_TOKEN",)"
        R"("message":"Connect the device again."}})") ==
      "Connect the device again.");
  ok1(SoaringScoring::ErrorMessage("not json").empty());
}

int
main()
{
  plan_tests(2 + 8 + 15 + 5);
  TestUrls();
  TestPilot();
  TestTasks();
  TestUploadAndError();
  return exit_status();
}
