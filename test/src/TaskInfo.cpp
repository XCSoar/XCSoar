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
  Args args(argc, argv, "FILE.tsk");
  tstring path = args.ExpectNextT();
  args.ExpectEnd();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  OrderedTask *task = LoadTask(path.c_str(), task_behaviour);
  if (task == NULL)
    return EXIT_FAILURE;

  Print(*task);

  delete task;
  return EXIT_SUCCESS;
}

