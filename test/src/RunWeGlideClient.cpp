// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "net/client/WeGlide/DownloadTask.hpp"
#include "net/client/WeGlide/ListTasks.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "net/client/WeGlide/UploadFlight.hpp"
#include "net/http/Init.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Serialiser.hpp"
#include "json/Serialize.hxx"
#include "co/Task.hxx"
#include "system/Args.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/StdioOutputStream.hxx"
#include "util/PrintException.hxx"

#include <boost/json.hpp>

#include <cstdio>

static constexpr auto usage = R"usage(
  task TASK_ID
  declaration USER_ID
  declarations
  by_user USER_ID
  competitions
  scores
  upload USER_ID BIRTHDAY GLIDER IGCFILE
)usage";

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static void
PrintTask(const OrderedTask &task)
{
  auto xml_node = XMLNode::CreateRoot("Task");
  WritableDataNodeXML data_node{xml_node};

  SaveTask(data_node, task);

  StdioOutputStream _stdout{stdout};
  BufferedOutputStream bos{_stdout};
  xml_node.Serialise(bos, true);
  bos.Flush();
}

int
main(int argc, char *argv[])
try {
  Args args{argc, argv, usage};
  const auto cmd = args.ExpectNext();

  WeGlideSettings settings;

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  Instance instance;

  ConsoleOperationEnvironment env;

  if (StringIsEqual(cmd, "task")) {
    const auto task_id = ParseUint64(args.ExpectNext());
    args.ExpectEnd();

    const auto task = instance.Run(
      WeGlide::DownloadTask(*Net::curl, settings,
                            task_id,
                            task_behaviour, nullptr,
                            env));
    if (!task)
      throw "No task";

    PrintTask(*task);
    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "declaration")) {
    settings.pilot_id = ParseUint64(args.ExpectNext());
    args.ExpectEnd();

    const auto task = instance.Run(
      WeGlide::DownloadDeclaredTask(*Net::curl, settings,
                                    task_behaviour, nullptr,
                                    env));
    if (!task)
      throw "No task";

    PrintTask(*task);
    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "declarations")) {
    args.ExpectEnd();

    const auto tasks = instance.Run(
      WeGlide::ListDeclaredTasks(*Net::curl, settings, env));

    for (const auto &task : tasks) {
      fmt::print("{}\t{}\t{:.1f} km\t{}\t{}\t{}\n",
                 task.id,
                 WeGlide::ToString(task.kind),
                 task.distance / 1000,
                 task.name,
                 task.user_name,
                 task.ruleset);

      for (const auto &tp : task.turnpoints)
        fmt::print("  {} ({} m)\n", tp.name, tp.elevation);
    }

    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "by_user")) {
    const uint_least64_t user_id = ParseUint64(args.ExpectNext());
    args.ExpectEnd();

    const auto tasks = instance.Run(
      WeGlide::ListTasksByUser(*Net::curl, settings, user_id, env));

    for (const auto &task : tasks) {
      fmt::print("{}\t{}\t{:.1f} km\t{}\t{}\t{}\n",
                 task.id,
                 WeGlide::ToString(task.kind),
                 task.distance / 1000,
                 task.name,
                 task.user_name,
                 task.ruleset);

      for (const auto &tp : task.turnpoints)
        fmt::print("  {} ({} m)\n", tp.name, tp.elevation);
    }

    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "competitions")) {
    args.ExpectEnd();

    const auto tasks = instance.Run(
      WeGlide::ListDailyCompetitions(*Net::curl, settings, env));

    if (tasks.empty()) {
      fmt::print("No competitions today.\n");
    } else {
      for (const auto &task : tasks) {
        fmt::print("{}\t{}\t{:.1f} km\t{}\t{}\t{}\n",
                   task.id,
                   WeGlide::ToString(task.kind),
                   task.distance / 1000,
                   task.name,
                   task.user_name,
                   task.ruleset);

        for (const auto &tp : task.turnpoints)
          fmt::print("  {} ({} m)\n", tp.name, tp.elevation);
      }
    }

    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "scores")) {
    args.ExpectEnd();

    const auto tasks = instance.Run(
      WeGlide::ListRecentTaskScores(*Net::curl, settings, env));

    if (tasks.empty()) {
      fmt::print("No recent scores.\n");
    } else {
      for (const auto &task : tasks) {
        fmt::print("{}\t{:.1f} km\t{}\t{}\n",
                   task.id,
                   task.distance / 1000,
                   task.name,
                   task.scoring_date);

        for (const auto &se : task.scores)
          fmt::print("  {:.0f} pts  {:.1f} km/h  {}\n",
                     se.points, se.speed, se.user_name);
      }
    }

    return EXIT_SUCCESS;
  } else if (StringIsEqual(cmd, "upload")) {
    settings.pilot_id = atoi(args.ExpectNext());
    const char *birthday_s = args.ExpectNext();
    const unsigned glider = atoi(args.ExpectNext());
    const auto igc_path = args.ExpectNextPath();
    args.ExpectEnd();

    unsigned year, month, day;
    if (sscanf(birthday_s, "%04u-%02u-%02u", &year, &month, &day) != 3)
      throw "Failed to parse date";

    settings.pilot_birthdate = {year, month, day};

    const auto value =
      instance.Run(WeGlide::UploadFlight(*Net::curl, settings, glider,
                                         igc_path, env));

    StdioOutputStream _stdout(stdout);
    Json::Serialize(_stdout, value);
    return EXIT_SUCCESS;
  } else
    throw "Unknown command";
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
