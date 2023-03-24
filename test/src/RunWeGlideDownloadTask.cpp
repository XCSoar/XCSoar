// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "net/client/WeGlide/DownloadTask.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "net/http/Init.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Serialiser.hpp"
#include "co/Task.hxx"
#include "system/Args.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/StdioOutputStream.hxx"
#include "util/PrintException.hxx"

#include <cstdio>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};

  std::unique_ptr<OrderedTask> value;

  Co::InvokeTask DoRun(const WeGlideSettings &settings,
                       const TaskBehaviour &task_behaviour,
                       const Waypoints *waypoints,
                       ProgressListener &progress)
  {
    value = co_await WeGlide::DownloadDeclaredTask(*Net::curl, settings,
                                                   task_behaviour, waypoints,
                                                   progress);
  }
};

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "PILOT");
  const unsigned pilot = atoi(args.ExpectNext());
  args.ExpectEnd();

  WeGlideSettings settings;
  settings.pilot_id = pilot;

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  Instance instance;
  ConsoleOperationEnvironment env;

  instance.Run(instance.DoRun(settings, task_behaviour, nullptr, env));

  if (!instance.value) {
    fprintf(stderr, "No task\n");
    return EXIT_FAILURE;
  }

  auto xml_node = XMLNode::CreateRoot(_T("Task"));
  WritableDataNodeXML data_node{xml_node};

  SaveTask(data_node, *instance.value);

  StdioOutputStream _stdout{stdout};
  BufferedOutputStream bos{_stdout};
  xml_node.Serialise(bos, true);
  bos.Flush();

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
