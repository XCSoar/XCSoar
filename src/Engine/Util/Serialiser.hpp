/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Task/Visitors/ObservationZoneVisitor.hpp"
#include "Task/Tasks/OrderedTask.hpp"

class DataNode;
class Waypoint;

class Serialiser:
  public TaskPointConstVisitor,
  public virtual ObservationZoneConstVisitor
{
public:
  Serialiser(DataNode& the_node): m_node(the_node) {};
  void serialise(const OrderedTask& data);
  void deserialise(OrderedTask& data);

  void Visit(const StartPoint& data);
  void Visit(const ASTPoint& data);
  void Visit(const AATPoint& data);
  void Visit(const FinishPoint& data);
  void Visit(const UnorderedTaskPoint& data);
  void Visit(const FAISectorZone& data);
  void Visit(const SectorZone& data);
  void Visit(const LineSectorZone& data);
  void Visit(const CylinderZone& data);

protected:
  void serialise(const OrderedTaskBehaviour& data);
  void deserialise(OrderedTaskBehaviour& data);

  void serialise(const Waypoint& data);
  Waypoint* deserialise_waypoint();

  void serialise(const GEOPOINT& data);
  void deserialise(GEOPOINT& data);

  void serialise(const ObservationZonePoint& data);
  ObservationZonePoint* deserialise_oz(const Waypoint& wp, const bool is_turnpoint);

  DataNode* serialise(const OrderedTaskPoint& data, const TCHAR* name);
  void deserialise_point(OrderedTask& data);

  DataNode &m_node;

private:
  OrderedTask::Factory_t task_factory_type() const;
  const TCHAR* task_factory_type(OrderedTask::Factory_t the_type) const;
};

#endif
