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

#include "DataNodeXML.hpp"
#include "Node.hpp"
#include "Util/StringAPI.hxx"

const TCHAR *
ConstDataNodeXML::GetName() const
{
  return node.GetName();
}

WritableDataNode*
WritableDataNodeXML::AppendChild(const TCHAR *name)
{
  return new WritableDataNodeXML(node.AddChild(name, false));
}

ConstDataNode *
ConstDataNodeXML::GetChildNamed(const TCHAR *name) const
{
  const XMLNode *child = node.GetChildNode(name);
  if (child == nullptr)
    return nullptr;

  return new ConstDataNodeXML(*child);
}

ConstDataNode::List
ConstDataNodeXML::ListChildren() const
{
  List list;
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    list.push_back(new ConstDataNodeXML(*i));
  return list;
}

ConstDataNode::List
ConstDataNodeXML::ListChildrenNamed(const TCHAR *name) const
{
  List list;
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    if (StringIsEqualIgnoreCase(i->GetName(), name))
      list.push_back(new ConstDataNodeXML(*i));
  return list;
}

void
WritableDataNodeXML::SetAttribute(const TCHAR *name, const TCHAR *value)
{
  node.AddAttribute(name, value);
}

const TCHAR *
ConstDataNodeXML::GetAttribute(const TCHAR *name) const
{
  return node.GetAttribute(name);
}
