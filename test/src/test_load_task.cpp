#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "Task/TaskManager.hpp"
#include <fstream>
#include <tchar.h>
#include <windef.h> /* for MAX_PATH */
#include "OS/PathName.hpp"
#include "Util/Deserialiser.hpp"
#include "Util/DataNodeXML.hpp"

static OrderedTask* task_load(OrderedTask* task) {
  PathName szFilename(task_file.c_str());
  DataNode *root = DataNodeXML::Load(szFilename);
  if (!root)
    return NULL;

  Deserialiser des(*root);
  des.deserialise(*task);
  if (task->CheckTask()) {
    delete root;
    return task;
  }
  delete task;
  delete root;
  return NULL;
}

static bool
test_load_task()
{
  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  OrderedTask *blank = new OrderedTask(task_behaviour);

  OrderedTask* t = task_load(blank);
  delete blank;
  return (t!= NULL);
}


int main(int argc, char** argv) 
{
  output_skip = 60;

  task_file = "test/data/apf-bug554.tsk";

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(1);

  ok(test_load_task(),"load task",0);

  return exit_status();
}

