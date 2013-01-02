/* Copyright_License {

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
#ifndef SERIALISER_HPP
#define SERIALISER_HPP

#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Factory/TaskFactoryType.hpp"
#include "Task/Ordered/OrderedTaskBehaviour.hpp"

#include <tchar.h>

class DataNode;
struct GeoPoint;
struct Waypoint;
class OrderedTask;
class OrderedTaskPoint;
class ObservationZonePoint;
class FAISectorZone;
class SectorZone;
class LineSectorZone;
class CylinderZone;
class KeyholeZone;
class BGAFixedCourseZone;
class BGAEnhancedOptionZone;
class BGAStartSectorZone;
class AnnularSectorZone;

/**
 * Class to serialise and de-serialise tasks to/from a #DataNode structure
 */
class Serialiser:
  public TaskPointConstVisitor
{
  DataNode &node;

  bool mode_optional_start;

public:
  /** 
   * Constructor
   * 
   * @param the_node Node this serialiser will edit
   * @param _waypoints a waypoint database to merge with (optional)
   * 
   * @return Initialised object
   */
  Serialiser(DataNode &_node)
    :node(_node), mode_optional_start(false) {};

  /** 
   * Serialise a task (create a DataNode structure to reflect the task)
   * 
   * @param data OrderedTask to serialise
   */
  void Serialise(const OrderedTask &task);

  void Visit(const StartPoint &data);
  void Visit(const ASTPoint &data);
  void Visit(const AATPoint &data);
  void Visit(const FinishPoint &data);
  void Visit(const UnorderedTaskPoint &data);
  void Visit(const FAISectorZone &data);
  void Visit(const KeyholeZone &data);
  void Visit(const BGAFixedCourseZone &data);
  void Visit(const BGAEnhancedOptionZone &data);
  void Visit(const BGAStartSectorZone &data);
  void Visit(const SectorZone &data);
  void Visit(const LineSectorZone &data);
  void Visit(const CylinderZone &data);
  void Visit(const AnnularSectorZone &data);

protected:
  /** 
   * Serialise OrderedTaskBehaviour
   * 
   * @param data Item to serialise
   */
  void Serialise(const OrderedTaskBehaviour &data);

  /** 
   * Serialise a Waypoint
   * 
   * @param data Item to serialise
   */
  void Serialise(const Waypoint &data);

  /** 
   * Serialise a GeoPoint
   * 
   * @param data Item to serialise
   */
  void Serialise(const GeoPoint &data);

  /** 
   * Serialise an ObservationZonePoint
   * 
   * @param data Item to serialise
   */
  void Serialise(const ObservationZonePoint &data);

  /** 
   * Serialise an OrderedTaskPoint
   * 
   * @param data Item to serialise
   * @param name Type of point
   */
  void Serialise(const OrderedTaskPoint &data, const TCHAR* name);

private:
  const TCHAR *GetTaskFactoryType(TaskFactoryType type) const;
  const TCHAR *GetHeightRef(HeightReferenceType height_ref) const;
};

#endif
