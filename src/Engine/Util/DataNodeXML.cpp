/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "DataNodeXML.hpp"
#include "IO/TextWriter.hpp"
#include "XML/Parser.hpp"

#include <stdio.h>

DataNode *
DataNodeXML::Load(const TCHAR* path)
{
  XMLNode *child = XML::OpenFileHelper(path);
  if (child == NULL)
    return NULL;

  DataNodeXML *data_node = new DataNodeXML(*child);
  delete child;
  return data_node;
}

DataNodeXML
DataNodeXML::CreateRoot(const TCHAR *node_name)
{
  XMLNode new_root = XMLNode::CreateRoot(node_name);
  return DataNodeXML(new_root);
}

void
DataNodeXML::Serialise(TextWriter &writer) const
{
  node.Serialise(writer, true);
}

const TCHAR *
DataNodeXML::GetName() const
{
  return node.GetName();
}

DataNode*
DataNodeXML::AppendChild(const TCHAR *name)
{
  return new DataNodeXML(node.AddChild(name, false));
}

DataNode *
DataNodeXML::GetChildNamed(const TCHAR *name) const
{
  const XMLNode *child = node.GetChildNode(name);
  if (child == NULL)
    return NULL;

  return new DataNodeXML(*child);
}

DataNode::List
DataNodeXML::ListChildren() const
{
  List list;
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    list.push_back(new DataNodeXML(*i));
  return list;
}

DataNode::List
DataNodeXML::ListChildrenNamed(const TCHAR *name) const
{
  List list;
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    if (_tcsicmp(i->GetName(), name) == 0)
      list.push_back(new DataNodeXML(*i));
  return list;
}

void
DataNodeXML::SetAttribute(const TCHAR *name, const TCHAR *value)
{
  node.AddAttribute(name, value);
}

bool
DataNodeXML::GetAttribute(const TCHAR *name, tstring &value) const
{
  const TCHAR *v = node.GetAttribute(name);
  if (v == NULL)
    return false;

  value.assign(v);
  return true;
}

bool
DataNodeXML::Save(const TCHAR *path)
{
  /// @todo make xml writing portable (unicode etc)
  TextWriter writer(path);
  if (writer.error())
    return false;

  Serialise(writer);
  return true;
}
