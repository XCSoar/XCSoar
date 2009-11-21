#ifndef TEST_TASK_HPP
#define TEST_TASK_HPP

#include "Task/TaskManager.hpp"

bool setup_task(TaskManager& task_manager, 
                const Waypoints& waypoints);

bool test_task(const Waypoints &waypoints);

bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_fai(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints);

#endif
