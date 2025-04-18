// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <boost/json/fwd.hpp>

#include <memory>

struct TaskBehaviour;
class OrderedTask;

std::unique_ptr<OrderedTask>
DecodeXCTrackTask(const boost::json::value &j,
                  const TaskBehaviour &task_behaviour);
