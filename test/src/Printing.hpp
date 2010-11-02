#ifndef PRINTING_HPP
#define PRINTING_HPP

#include "Engine/Task/TaskManager.hpp"
#include "Task/Tasks/AbortTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbstractTask.hpp"

std::ostream& operator<<(std::ostream& os,fixed const& value);

class PrintHelper {
public:
  static void taskmanager_print(TaskManager& task, const AIRCRAFT_STATE &location);
  static void abstracttask_print(AbstractTask& task, const AIRCRAFT_STATE &location);
  static void aborttask_print(AbortTask& task, const AIRCRAFT_STATE &location);
  static void gototask_print(GotoTask& task, const AIRCRAFT_STATE &location);
  static void orderedtask_print(OrderedTask& task, const AIRCRAFT_STATE &location);
};

#endif
