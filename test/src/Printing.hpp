// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define DO_PRINT

#include <iostream>

class Path;
class TaskManager;
class AbortTask;
class GotoTask;
class OrderedTask;
class AbstractTask;
class TaskPoint;
class SampledTaskPoint;
class ScoredTaskPoint;
class OrderedTaskPoint;
class ContestManager;
class Trace;
class AATPoint;
class TaskProjection;
struct AircraftState;
struct GeoPoint;
struct ContestResult;
class RoutePlanner;
class TerrainRoute;
class ReachFan;
class FlatTriangleFanTree;
class FlatTriangleFan;
struct Waypoint;
struct AirspaceAltitude;

std::ostream &operator<<(std::ostream &f, Path path);
std::ostream &operator<< (std::ostream &f, const Waypoint &wp);

class PrintHelper {
public:
  static void taskmanager_print(const TaskManager &task,
                                const AircraftState &location);
  static void abstracttask_print(const AbstractTask &task,
                                 const AircraftState &location);
  static void aborttask_print(const AbortTask &task,
                              const AircraftState &location);
  static void gototask_print(const GotoTask &task,
                             const AircraftState &location);
  static void orderedtask_print(const OrderedTask &task,
                                const AircraftState &location);
  static void taskpoint_print(std::ostream& f, const TaskPoint& tp, const AircraftState &state);
  static void sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp, 
                                     const AircraftState &state);
  static void sampledtaskpoint_print_samples(std::ostream &f,
                                             const ScoredTaskPoint &tp,
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
  static void contestmanager_print(const ContestManager& cm,
                                   const Trace &trace_full,
                                   const Trace &trace_triangle,
                                   const Trace &trace_sprint);
  static void trace_print(const Trace& trace, const GeoPoint &loc);
  static void print(const ContestResult& result);
  static void print_route(RoutePlanner& r);
  static void print(const ReachFan& r);
  static void print(const FlatTriangleFanTree& r);
  static void print(const FlatTriangleFan& r, const unsigned depth);
};
