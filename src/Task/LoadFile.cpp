/* Copyright_License {

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

#include "LoadFile.hpp"
#include "Deserialiser.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Parser.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "OS/Path.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StringAPI.hxx"

#include <memory>

#include <tchar.h>

OrderedTask *
LoadTask(Path path, const TaskBehaviour &task_behaviour,
         const Waypoints *waypoints)
{
  // Load root node
  std::unique_ptr<XMLNode> xml_root(XML::ParseFile(path));
  if (!xml_root)
    return nullptr;

  const ConstDataNodeXML root(*xml_root);

  // Check if root node is a <Task> node
  if (!StringIsEqual(root.GetName(), _T("Task")))
    return nullptr;

  // Create a blank task
  OrderedTask *task = new OrderedTask(task_behaviour);

  // Read the task from the XML file
  LoadTask(*task, root, waypoints);

  // Return the parsed task
  return task;
}
