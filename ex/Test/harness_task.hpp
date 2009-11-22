#ifndef TEST_TASK_HPP
#define TEST_TASK_HPP

#include "Task/TaskManager.hpp"

bool setup_task(TaskManager& task_manager, 
                const Waypoints& waypoints);

bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_fai(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints);

bool test_task_or(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_dash(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_manip(TaskManager& task_manager,
                     const Waypoints &waypoints);

void task_report(TaskManager& task_manager, const char* text);

#endif
