#include "test_debug.hpp"
#include "harness_aircraft.hpp"
#include "TaskEventsPrint.hpp"
#include "Task/TaskManager.hpp"
#include "UtilsText.hpp"
#include <fstream>
#include <tchar.h>
#include <windef.h> /* for MAX_PATH */
#include "OS/PathName.hpp"
#include "Util/Deserialiser.hpp"
#include "Util/DataNodeXML.hpp"

static OrderedTask* task_load(OrderedTask* task) {
  TCHAR szFilename[MAX_PATH];
  ConvertCToT(szFilename, task_file.c_str());
  DataNode* root = DataNodeXML::load(szFilename);
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
  GlidePolar glide_polar(fixed(4.0));
  TaskBehaviour task_behaviour;
  TaskEventsPrint default_events(verbose);

  OrderedTask* blank = 
    new OrderedTask(default_events, task_behaviour, glide_polar);

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

