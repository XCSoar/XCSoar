/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_MAT_POINTS_HPP
#define XCSOAR_MAT_POINTS_HPP

#include <vector>

class Waypoints;
class AbstractTaskFactory;
class OrderedTaskPoint;
struct TaskBehaviour;
struct OrderedTaskBehaviour;

class MatPoints {
public:
  /**
   * Vector of Turnpoints of all waypoints marked as Turnpoints
   */
  typedef std::vector<OrderedTaskPoint *> MatVector;

private:
  /**
   * the list of mat points
   */
  MatVector points;

public:
  MatPoints() = default;
  MatPoints(const MatPoints &other) = delete;
  MatPoints &operator=(const MatPoints &other) = delete;

  /**
   * populates the mat_points with the first 128 turnpoints in the waypoint file
   * @param waypoints.  The list of active waypoints
   */
  void FillMatPoints(const Waypoints &wps, const AbstractTaskFactory &factory);

  /**
   * removes all points from mat_points
   */
  void ClearMatPoints();

  const MatVector &GetMatPoints() const {
    return points;
  }

  MatVector &SetMatPoints() {
    return points;
  }

  void CloneFrom(const MatPoints &other,
                 const TaskBehaviour &task_behaviour,
                 const OrderedTaskBehaviour &ordered_task_behaviour);
};

#endif
