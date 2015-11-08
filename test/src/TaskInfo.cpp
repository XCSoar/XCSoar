#include "OS/Args.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Task/LoadFile.hpp"

#include <tchar.h>

static OrderedTask *
LoadTask2(Path path, const TaskBehaviour &task_behaviour)
{
  OrderedTask *task = LoadTask(path, task_behaviour);
  if (task == nullptr) {
    fprintf(stderr, "Failed to parse XML\n");
    return nullptr;
  }

  task->UpdateGeometry();
  if (!task->CheckTask()) {
    fprintf(stderr, "Failed to load task from XML\n");
    delete task;
    return NULL;
  }

  return task;
}

static void
Print(const TaskStats &stats)
{
  printf("distance nominal = %f km\n", double(stats.distance_nominal / 1000));
  printf("distance min = %f km\n", double(stats.distance_min / 1000));
  printf("distance max = %f km\n", double(stats.distance_max / 1000));
}

static void
Print(const OrderedTask &task)
{
  Print(task.GetStats());
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.tsk ...");
  if (args.IsEmpty())
    args.UsageError();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  int result = EXIT_SUCCESS;

  do {
    const auto path = args.ExpectNextPath();
    OrderedTask *task = LoadTask2(path, task_behaviour);
    if (task != NULL) {
      Print(*task);
      delete task;
    } else {
      _ftprintf(stderr, _T("Failed to load %s\n"), path.c_str());
      result = EXIT_FAILURE;
    }

  } while (!args.IsEmpty());

  return result;
}
