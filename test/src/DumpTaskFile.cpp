// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Serialiser.hpp"
#include "Task/TaskFile.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/StdioOutputStream.hxx"
#include "util/PrintException.hxx"

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.tsk|cup|igc [IDX]");
  const auto path = args.ExpectNextPath();
  int idx = -1;
  if (!args.IsEmpty())
    idx = args.ExpectNextInt();
  args.ExpectEnd();

  const auto file = TaskFile::Create(path);
  if (!file) {
    fprintf(stderr, "TaskFile::Create() failed\n");
    return EXIT_FAILURE;
  }

  if (idx >= 0) {
    TaskBehaviour task_behaviour;
    task_behaviour.SetDefaults();

    const auto task = file->GetTask(task_behaviour, nullptr, idx);
    if (task == nullptr)
      throw "No such task";

    auto xml_node = XMLNode::CreateRoot("Task");
    WritableDataNodeXML data_node{xml_node};

    SaveTask(data_node, *task);

    StdioOutputStream _stdout{stdout};
    WithBufferedOutputStream(_stdout, [&xml_node](auto &bos){
      xml_node.Serialise(bos, true);
    });
  } else {
    for (const auto &i : file->GetList())
      printf("task: %s\n", i.c_str());
  }

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}

