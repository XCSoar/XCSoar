// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StartConstraints.hpp"
#include "FinishConstraints.hpp"
#include "Task/Shapes/FAITriangleSettings.hpp"

#include <chrono>

/**
 * Settings for ordered tasks; most of these are set by
 * the #AbstractTaskFactory but can be overriden
 */
struct OrderedTaskSettings {
  /** Desired AAT minimum task time (s) */
  std::chrono::duration<unsigned> aat_min_time;

  StartConstraints start_constraints;
  FinishConstraints finish_constraints;

  FAITriangleSettings fai_triangle;

  void SetDefaults();
};
