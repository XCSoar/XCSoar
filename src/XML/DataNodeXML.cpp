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

#include "DataNodeXML.hpp"
#include "Util/StringUtil.hpp"
#include "IO/TextWriter.hpp"
#include "XML/Parser.hpp"

#include <memory>

DataNode *
DataNodeXML::Load(const TCHAR* path)
{
  std::unique_ptr<XMLNode> child(XML::OpenFileHelper(path));
  if (!child)
    return NULL;

  return new DataNodeXML(std::move(*child));
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
    if (StringIsEqualIgnoreCase(i->GetName(), name))
      list.push_back(new DataNodeXML(*i));
  return list;
}

void
DataNodeXML::SetAttribute(const TCHAR *name, const TCHAR *value)
{
  node.AddAttribute(name, value);
}

const TCHAR *
DataNodeXML::GetAttribute(const TCHAR *name) const
{
  return node.GetAttribute(name);
}

bool
DataNodeXML::Save(const TCHAR *path)
{
  /// @todo make xml writing portable (unicode etc)
  TextWriter writer(path);
  if (!writer.IsOpen())
    return false;

  Serialise(writer);
  return true;
}
