// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LoadFile.hpp"
#include "Deserialiser.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Parser.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "system/Path.hpp"
#include "util/StringUtil.hpp"
#include "util/StringAPI.hxx"

#include <stdexcept>

#include <tchar.h>

std::unique_ptr<OrderedTask>
LoadTask(Path path, const TaskBehaviour &task_behaviour,
         const Waypoints *waypoints)
{
  // Load root node
  const auto xml_root = XML::ParseFile(path);
  const ConstDataNodeXML root(xml_root);

  // Check if root node is a <Task> node
  if (!StringIsEqual(root.GetName(), "Task"))
    throw std::runtime_error("Invalid task file");

  // Create a blank task
  auto task = std::make_unique<OrderedTask>(task_behaviour);

  // Read the task from the XML file
  LoadTask(*task, root, waypoints);

  // Return the parsed task
  return task;
}
