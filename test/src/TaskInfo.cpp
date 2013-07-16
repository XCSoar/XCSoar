#include "OS/Args.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "XML/DataNodeXML.hpp"
#include "Task/Deserialiser.hpp"

static OrderedTask *
LoadTask(const TCHAR *path, const TaskBehaviour &task_behaviour)
{
  DataNode *node = DataNodeXML::Load(path);
  if (node == NULL) {
    fprintf(stderr, "Failed to parse XML\n");
    return NULL;
  }

  Deserialiser des(*node);
  OrderedTask *task = new OrderedTask(task_behaviour);
  des.Deserialise(*task);
  delete node;

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
    tstring path = args.ExpectNextT();
    OrderedTask *task = LoadTask(path.c_str(), task_behaviour);
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

