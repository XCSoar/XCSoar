// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Task/TaskManager.hpp"

static constexpr std::size_t NUM_TASKS = 5;

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

/**
 * creates a random task of type (RT/AAT/FAI)
 * changes it to a random type of (RT/AAT/FAI)
 * mutates and validates the resulting tp types
 */
bool test_task_type_manip(TaskManager& task_manager,
                     const Waypoints &waypoints, unsigned n_points);

bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_random(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points);

/**
 * used for testing xcsoar6.1 that only has three types
 * of tasks RT, AAT, FAI
 * Generates random task type with valid random
 * start/finish/intermediate points, sizes etc
 */
bool test_task_random_RT_AAT_FAI(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points);

bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num);

bool test_task_bad(TaskManager& task_manager,
                   const Waypoints &waypoints);

void test_note(const char* text);

void
task_report(const TaskManager &task_manager, const char *text);

const char* task_name(int test_num);

WaypointPtr random_waypoint(const Waypoints &waypoints);
