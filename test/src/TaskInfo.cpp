#include "system/Args.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/LoadFile.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "util/PrintException.hxx"

#include <cassert>

#include <tchar.h>

static std::unique_ptr<OrderedTask>
LoadTask2(Path path, const TaskBehaviour &task_behaviour)
{
  auto task = LoadTask(path, task_behaviour);
  assert(task);

  task->UpdateGeometry();

  const auto errors = task->CheckTask();
  if (!errors.IsEmpty())
    _fputts(getTaskValidationErrors(errors), stderr);

  if (IsError(errors)) {
    fprintf(stderr, "Failed to load task from XML\n");
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
try {
  Args args(argc, argv, "FILE.tsk ...");
  if (args.IsEmpty())
    args.UsageError();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  int result = EXIT_SUCCESS;

  do {
    const auto path = args.ExpectNextPath();
    const auto task = LoadTask2(path, task_behaviour);
    if (task != NULL) {
      Print(*task);
    } else {
      _ftprintf(stderr, _T("Failed to load %s\n"), path.c_str());
      result = EXIT_FAILURE;
    }

  } while (!args.IsEmpty());

  return result;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
