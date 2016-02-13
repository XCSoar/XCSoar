/*
 * Copyright (C) 2011 Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KINETIC_MANAGER_HPP
#define KINETIC_MANAGER_HPP

#include "Time/PeriodClock.hpp"

/**
 * A manager class that can be used for kinetic scrolling
 */
class KineticManager
{
  /** Time in ms until the kinetic movement is stopped */
  const int stopping_time;

  /** Whether the kinetic movement is still active */
  bool steady;

  /** Position at the end of the manual movement */
  int last;

  /** Precalculated final position of the kinetic movement */
  int end;

  /** Speed at the end of the manual movement */
  double v;

  /** Clock that is used for the kinetic movement */
  PeriodClock clock;

public:
  KineticManager(int _stopping_time = 1000)
    :stopping_time(_stopping_time), steady(true) {}

  /** Needs to be called once the manual movement is started */
  void MouseDown(int x);
  /** Needs to be called on every manual mouse move event */
  void MouseMove(int x);
  /**
   * Needs to be called at the end of the manual movement and
   * starts the kinetic movement
   */
  void MouseUp(int x);

  /**
   * Returns the current position of the kinetic movement.
   * Sets the steady flag to true if the kinetic motion is
   * not active (@see IsSteady())
   */
  int GetPosition();

  /** Returns whether the kinetic movement is still active */
  bool IsSteady();
};

#endif
