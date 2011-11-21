/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
class TaskProjection;
struct AircraftState;
struct GeoPoint;
struct ContestResult;
class RoutePlanner;
class ReachFan;
class FlatTriangleFanTree;
class FlatTriangleFan;

#ifdef FIXED_MATH
#include "Math/fixed.hpp"
std::ostream& operator<<(std::ostream& os,fixed const& value);
#endif

class PrintHelper {
public:
  static void taskmanager_print(TaskManager& task, const AircraftState &location);
  static void abstracttask_print(AbstractTask& task, const AircraftState &location);
  static void aborttask_print(AbortTask& task, const AircraftState &location);
  static void gototask_print(GotoTask& task, const AircraftState &location);
  static void orderedtask_print(OrderedTask& task, const AircraftState &location);
  static void taskpoint_print(std::ostream& f, const TaskPoint& tp, const AircraftState &state);
  static void sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp, 
                                     const AircraftState &state);
  static void sampledtaskpoint_print_samples(std::ostream& f, const SampledTaskPoint& tp, 
                                             const AircraftState &state);
  static void orderedtaskpoint_print(std::ostream& f, const OrderedTaskPoint& tp, 
                                     const AircraftState &state,
                                     const int item=0);
  static void orderedtaskpoint_print_boundary(std::ostream& f, const OrderedTaskPoint& tp, 
                                              const AircraftState &state);
  static void aatpoint_print(std::ostream& f, const AATPoint& tp, 
                             const AircraftState &state,
                             const TaskProjection &projection,
                             const int item=0);
  static void contestmanager_print(const ContestManager& cm);
  static void trace_print(const Trace& trace, const GeoPoint &loc);
  static void print(const ContestResult& result);
  static void print_route(RoutePlanner& r);
  static void print_reach_tree(const RoutePlanner& r);
  static void print(const ReachFan& r);
  static void print(const FlatTriangleFanTree& r);
  static void print(const FlatTriangleFan& r, const unsigned depth);
};

struct FlatGeoPoint;
class SearchPoint;
class SearchPointVector;
class AbstractAirspace;

void write_point(const SearchPoint& sp, const FlatGeoPoint& p, const char* name);
void write_spv (const SearchPointVector& spv);
void write_border (const AbstractAirspace& as);

#endif
