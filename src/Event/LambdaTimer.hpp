/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_EVENT_LAMBDA_TIMER_HPP
#define XCSOAR_EVENT_LAMBDA_TIMER_HPP

#include "Timer.hpp"
#include "Compiler.h"

#include <utility>

/**
 * An adapter that forwards Timer::OnTimer() calls to a lambda
 * expression.
 */
template<typename C>
class LambdaTimer : public Timer, private C {
public:
  LambdaTimer(C &&c):C(std::move(c)) {}

  using Timer::IsActive;
  using Timer::Schedule;
  using Timer::Cancel;

protected:
  virtual void OnTimer() override {
    C::operator()();
  }
};

/**
 * Convert a lambda expression (a closure object) to a Timer.  This is
 * a convenience function.
 */
template<typename C>
LambdaTimer<C>
MakeLambdaTimer(C &&c)
{
  return LambdaTimer<C>(std::move(c));
}

#endif
