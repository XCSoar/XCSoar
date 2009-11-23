#ifndef TEST_TASK_HPP
#define TEST_TASK_HPP

#include "Task/TaskManager.hpp"

#define NUM_TASKS 5

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

bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num);

void task_report(TaskManager& task_manager, const char* text);

const char* task_name(int test_num);

#endif
