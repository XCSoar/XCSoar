/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef PRINTING_HPP
#define PRINTING_HPP

#include <iostream>

class TaskManager;
class AbortTask;
class GotoTask;
class OrderedTask;
class AbstractTask;
class TaskPoint;
class SampledTaskPoint;
class OrderedTaskPoint;
class ContestManager;
class Trace;
class AATPoint;
struct AIRCRAFT_STATE;
struct GeoPoint;
struct ContestResult;

#ifdef FIXED_MATH
#include "Math/fixed.hpp"
std::ostream& operator<<(std::ostream& os,fixed const& value);
#endif

class PrintHelper {
public:
  static void taskmanager_print(TaskManager& task, const AIRCRAFT_STATE &location);
  static void abstracttask_print(AbstractTask& task, const AIRCRAFT_STATE &location);
  static void aborttask_print(AbortTask& task, const AIRCRAFT_STATE &location);
  static void gototask_print(GotoTask& task, const AIRCRAFT_STATE &location);
  static void orderedtask_print(OrderedTask& task, const AIRCRAFT_STATE &location);
  static void taskpoint_print(std::ostream& f, const TaskPoint& tp, const AIRCRAFT_STATE &state);
  static void sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp, 
                                     const AIRCRAFT_STATE &state);
  static void sampledtaskpoint_print_samples(std::ostream& f, const SampledTaskPoint& tp, 
                                             const AIRCRAFT_STATE &state);
  static void orderedtaskpoint_print(std::ostream& f, const OrderedTaskPoint& tp, 
                                     const AIRCRAFT_STATE &state,
                                     const int item=0);
  static void orderedtaskpoint_print_boundary(std::ostream& f, const OrderedTaskPoint& tp, 
                                              const AIRCRAFT_STATE &state);
  static void aatpoint_print(std::ostream& f, const AATPoint& tp, 
                             const AIRCRAFT_STATE &state,
                             const int item=0);
  static void contestmanager_print(const ContestManager& cm);
  static void trace_print(const Trace& trace, const GeoPoint &loc);
  static void print(const ContestResult& result);
};

#endif
