/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
