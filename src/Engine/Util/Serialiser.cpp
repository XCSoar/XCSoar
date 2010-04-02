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

#include "Serialiser.hpp"
#include "Task/OrderedTaskBehaviour.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "DataNode.hpp"
#include <assert.h>


///////////////////////////////////////////////////////////

void
Serialiser::deserialise_point(OrderedTask& data)
{
  tstring type;
  if (!m_node.get_attribute(_T("type"),type)) {
    assert(1);
    return;
  }

  DataNode* wp_node = m_node.get_child_by_name(_T("Waypoint"));
  if (wp_node==NULL)
    return;
  Serialiser wser(*wp_node);
  Waypoint *wp = wser.deserialise_waypoint();
  if (wp==NULL) {
    delete wp_node;
    return;
  }

  DataNode* oz_node = m_node.get_child_by_name(_T("ObservationZone"));
  if (oz_node==NULL) {
    delete wp_node;
    delete wp;
    return;
  }
  Serialiser oser(*oz_node);

  AbstractTaskFactory& fact = data.get_factory();

  ObservationZonePoint* oz = NULL;
  OrderedTaskPoint *pt = NULL;

  if (_tcscmp(type.c_str(), _T("Start")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, false)) != NULL) {
      pt = fact.createStart(oz, *wp);
    }
  } else if (_tcscmp(type.c_str(), _T("Turn")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, true)) != NULL) {
      pt = fact.createAST(oz, *wp);
    }
  } else if (_tcscmp(type.c_str(), _T("Area")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, true)) != NULL) {
      pt = fact.createAAT(oz, *wp);
    }
  } else if (_tcscmp(type.c_str(), _T("Finish")) == 0) {
    if ((oz = oser.deserialise_oz(*wp, true)) != NULL) {
      pt = fact.createFinish(oz, *wp);
    }
  } 

  if (pt != NULL) {
    fact.append(pt,false);
  }

  delete wp;
  delete wp_node;
  delete oz_node;
}


void
Serialiser::Visit(const StartPoint& data)
{
  DataNode* child = serialise(data, _T("Start"));
  delete child;
}

void
Serialiser::Visit(const ASTPoint& data)
{
  DataNode* child = serialise(data, _T("Turn"));
  delete child;
}

void
Serialiser::Visit(const AATPoint& data)
{
  DataNode* child = serialise(data, _T("Area"));
  delete child;
}

void
Serialiser::Visit(const FinishPoint& data)
{
  DataNode* child = serialise(data, _T("Finish"));
  delete child;
}

void
Serialiser::Visit(const UnorderedTaskPoint& data)
{
  // visit(data, _T("Unordered"));
}


DataNode*
Serialiser::serialise(const OrderedTaskPoint& data, const TCHAR* name)
{
  // do nothing
  DataNode* child = m_node.add_child(_T("Point"));
  child->set_attribute(_T("type"), name);

  DataNode* wchild = child->add_child(_T("Waypoint"));
  Serialiser wser(*wchild);
  wser.serialise(data.get_waypoint());
  delete wchild;

  DataNode* ochild = child->add_child(_T("ObservationZone"));
  Serialiser oser(*ochild);
  oser.serialise(*data.get_oz());
  delete ochild;

  return child;
}

///////////////////////////////////////////////////

ObservationZonePoint*
Serialiser::deserialise_oz(const Waypoint& wp, const bool is_turnpoint)
{

  tstring type;
  if (!m_node.get_attribute(_T("type"),type)) {
    assert(1);
    return NULL;
  }
  if (_tcscmp(type.c_str(), _T("Line")) == 0) {
    LineSectorZone *ls = new LineSectorZone(wp.Location);

    fixed length;
    if (m_node.get_attribute(_T("length"), length)) {
      ls->setLength(length);
    }
    return ls;
  } else if (_tcscmp(type.c_str(), _T("Cylinder")) == 0) {
    CylinderZone *ls = new CylinderZone(wp.Location);

    fixed radius;
    if (m_node.get_attribute(_T("radius"), radius)) {
      ls->setRadius(radius);
    }
    return ls;
  } else if (_tcscmp(type.c_str(), _T("Sector")) == 0) {
    SectorZone *ls = new SectorZone(wp.Location);

    fixed radius, start, end;
    if (m_node.get_attribute(_T("radius"), radius)) {
      ls->setRadius(radius);
    }
    if (m_node.get_attribute(_T("start_radial"), start)) {
      ls->setStartRadial(start);
    }
    if (m_node.get_attribute(_T("end_radial"), end)) {
      ls->setEndRadial(end);
    }
    return ls;
  } else if (_tcscmp(type.c_str(), _T("FAISector")) == 0) {
    return new FAISectorZone(wp.Location, is_turnpoint);
  }
  assert(1);
  return NULL;
}


void 
Serialiser::serialise(const ObservationZonePoint& data) 
{
  data.CAccept(*(ObservationZoneConstVisitor*)this);
} 


void 
Serialiser::Visit(const FAISectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("FAISector"));
}

void 
Serialiser::Visit(const SectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("Sector"));
  m_node.set_attribute(_T("radius"), data.getRadius());
  m_node.set_attribute(_T("start_radial"), data.getStartRadial());
  m_node.set_attribute(_T("end_radial"), data.getEndRadial());
}

void 
Serialiser::Visit(const LineSectorZone& data)
{
  m_node.set_attribute(_T("type"), _T("Line"));
  m_node.set_attribute(_T("length"), data.getLength());
}

void 
Serialiser::Visit(const CylinderZone& data)
{
  m_node.set_attribute(_T("type"), _T("Cylinder"));
  m_node.set_attribute(_T("radius"), data.getRadius());
}


///////////////////


void 
Serialiser::serialise(const GEOPOINT& data)
{
  m_node.set_attribute(_T("longitude"), data.Longitude);
  m_node.set_attribute(_T("latitude"), data.Latitude);
}

void 
Serialiser::deserialise(GEOPOINT& data)
{
  m_node.get_attribute(_T("longitude"), data.Longitude);
  m_node.get_attribute(_T("latitude"), data.Latitude);
}

////////////////////

Waypoint*
Serialiser::deserialise_waypoint()
{
  DataNode* loc_node = m_node.get_child_by_name(_T("Location"));
  if (!loc_node)
    return NULL;
  GEOPOINT loc;
  Serialiser lser(*loc_node);
  lser.deserialise(loc);
  delete loc_node;

  /// @todo: check if waypoint is already in database or needs to be added

  Waypoint *wp = new Waypoint();
  m_node.get_attribute(_T("name"), wp->Name);
  m_node.get_attribute(_T("id"), wp->id);
  m_node.get_attribute(_T("comment"), wp->Comment);
  m_node.get_attribute(_T("altitude"), wp->Altitude);

  wp->Location = loc;

  return wp;
}

void 
Serialiser::serialise(const Waypoint& data)
{
  m_node.set_attribute(_T("name"), data.Name);
  m_node.set_attribute(_T("id"), data.id);
  m_node.set_attribute(_T("comment"), data.Comment);
  m_node.set_attribute(_T("altitude"), data.Altitude);

  DataNode* child = m_node.add_child(_T("Location"));
  Serialiser ser(*child);
  ser.serialise(data.Location);
  delete child;
}



//////////////////////////////////////////////////////////////////

void 
Serialiser::serialise(const OrderedTaskBehaviour& data)
{
  m_node.set_attribute(_T("task_scored"), data.task_scored);
  m_node.set_attribute(_T("aat_min_time"), data.aat_min_time);
  m_node.set_attribute(_T("start_max_speed"), data.start_max_speed);
  m_node.set_attribute(_T("start_max_height"), data.start_max_height);
  m_node.set_attribute(_T("start_max_height_ref"), data.start_max_height_ref);
  m_node.set_attribute(_T("finish_min_height"), data.finish_min_height);
  m_node.set_attribute(_T("fai_finish"), data.fai_finish);
  m_node.set_attribute(_T("min_points"), data.min_points);
  m_node.set_attribute(_T("max_points"), data.max_points);
  m_node.set_attribute(_T("homogeneous_tps"), data.homogeneous_tps);
  m_node.set_attribute(_T("is_closed"), data.is_closed);
}

void 
Serialiser::deserialise(OrderedTaskBehaviour& data)
{
  m_node.get_attribute(_T("task_scored"), data.task_scored);
  m_node.get_attribute(_T("aat_min_time"), data.aat_min_time);
  m_node.get_attribute(_T("start_max_speed"), data.start_max_speed);
  m_node.get_attribute(_T("start_max_height"), data.start_max_height);
  m_node.get_attribute(_T("start_max_height_ref"), data.start_max_height_ref);
  m_node.get_attribute(_T("finish_min_height"), data.finish_min_height);
  m_node.get_attribute(_T("fai_finish"), data.fai_finish);
  m_node.get_attribute(_T("min_points"), data.min_points);
  m_node.get_attribute(_T("max_points"), data.max_points);
  m_node.get_attribute(_T("homogeneous_tps"), data.homogeneous_tps);
  m_node.get_attribute(_T("is_closed"), data.is_closed);
}

/////////////////////////////////////////////////////

void 
Serialiser::serialise(const OrderedTask& data)
{
  m_node.set_attribute(_T("type"), task_factory_type(data.get_factory_type()));
  serialise(data.get_ordered_task_behaviour());
  data.tp_CAccept(*this);
}


void 
Serialiser::deserialise(OrderedTask& data)
{
  data.clear();
  data.set_factory(task_factory_type());
  data.reset();

  OrderedTaskBehaviour beh = data.get_ordered_task_behaviour();
  deserialise(beh);
  data.set_ordered_task_behaviour(beh);

  DataNode* point_node;
  unsigned i=0;
  while ((point_node = m_node.get_child_by_name(_T("Point"),i)) != NULL) {
    Serialiser pser(*point_node);
    pser.deserialise_point(data);
    delete point_node;
    i++;
  }
}


OrderedTask::Factory_t 
Serialiser::task_factory_type() const
{
  tstring type;
  if (!m_node.get_attribute(_T("type"),type)) {
    assert(1);
    return OrderedTask::FACTORY_FAI;
  }
  if (_tcscmp(type.c_str(), _T("FAI")) == 0) {
    return OrderedTask::FACTORY_FAI;
  } else if (_tcscmp(type.c_str(), _T("FAI")) == 0) {
    return OrderedTask::FACTORY_FAI;
  } else if (_tcscmp(type.c_str(), _T("RT")) == 0) {
    return OrderedTask::FACTORY_RT;
  } else if (_tcscmp(type.c_str(), _T("AAT")) == 0) {
    return OrderedTask::FACTORY_AAT;
  } else if (_tcscmp(type.c_str(), _T("Mixed")) == 0) {
    return OrderedTask::FACTORY_MIXED;
  } else if (_tcscmp(type.c_str(), _T("Touring")) == 0) {
    return OrderedTask::FACTORY_TOURING;
  }
  assert(1);
  return OrderedTask::FACTORY_FAI;
}

const TCHAR* 
Serialiser::task_factory_type(OrderedTask::Factory_t the_type) const
{
  switch(the_type) {
  case OrderedTask::FACTORY_FAI:
    return _T("FAI");
  case OrderedTask::FACTORY_RT:
    return _T("RT");
  case OrderedTask::FACTORY_AAT:
    return _T("AAT");
  case OrderedTask::FACTORY_MIXED:
    return _T("Mixed");
  case OrderedTask::FACTORY_TOURING:
    return _T("Touring");
  };
  assert(1);
  return NULL;
}


///////////////////////////////////////////////////////////////////
#include "DataNodeXML.hpp"

bool test_serialiser_save(const char* path, const OrderedTask& task)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("Task"));

  /*
  DataNode* child;
  child = root->add_child(_T("Task"));
  Serialiser ser(*child);
  ser.serialise(task);
  delete child;
  */
  Serialiser tser(*root);
  tser.serialise(task);

  printf("CREATED\n%S", root->serialise().c_str());

  bool retval = false;
  if (!root->save(path)) {
    printf("can't save\n");
  } else {
    retval = true;
  }
  delete root;  
  return retval;

  return true;
}

bool test_serialiser_load(const char* path, OrderedTask& task)
{
  DataNode* root = DataNodeXML::load(path);
  if (!root) {
    printf("can't load\n");
    return false;
  }
  if (_tcscmp(root->get_name().c_str(),_T("Task"))==0) {
    Serialiser des(*root);
    des.deserialise(task);

    printf("LOADED\n%S", root->serialise().c_str());
  } else {
    printf("can't find task\n");
  }

  delete root;

  return true;
}

/////////////


bool test_datanodexml_save(const char* path)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("root"));
  DataNode* child;

  child = root->add_child(_T("child1"));
  child->set_attribute(_T("lab1"),_T("val1"));
  child->set_attribute(_T("lab2"),_T("val2"));
  delete child;

  child = root->add_child(_T("child2"));
  child->set_attribute(_T("lab3"),_T("val3"));
  delete child;

  printf("CREATED\n%S", root->serialise().c_str());

  bool retval = false;
  if (!root->save(path)) {
    printf("can't save\n");
  } else {
    retval = true;
  }
  delete root;  
  return retval;
}

bool test_datanodexml_load(const char* path)
{
  DataNode* root = DataNodeXML::load(path);
  if (!root) {
    printf("can't load\n");
    return false;
  }
  printf("LOADED\n%S", root->serialise().c_str());

  delete root;

  return true;
}

void test_datanodexml(OrderedTask& task)
{
  char path[] = "test.xml";

//  test_datanodexml_save(path);
//  test_datanodexml_load(path);
//  test_serialiser_save(path, task);
  test_serialiser_load(path, task);
}


