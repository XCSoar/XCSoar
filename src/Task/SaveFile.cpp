// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SaveFile.hpp"
#include "Serialiser.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Node.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "system/Path.hpp"

#include <tchar.h>

void
SaveTask(Path path, const OrderedTask &task)
{
  XMLNode root_node = XMLNode::CreateRoot("Task");

  {
    WritableDataNodeXML root(root_node);
    SaveTask(root, task);
  }

  FileOutputStream file(path);
  BufferedOutputStream buffered(file);
  root_node.Serialise(buffered, true);
  buffered.Flush();
  file.Commit();
}
