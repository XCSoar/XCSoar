/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "OS/PathName.hpp"
#include "IO/TextWriter.hpp"
#include "xmlParser.hpp"

#include <stdio.h>

DataNodeXML::DataNodeXML(const XMLNode &the_node)
{
  m_xml_node = new XMLNode(the_node);
}

DataNodeXML::~DataNodeXML()
{
  delete m_xml_node;
}

DataNode* 
DataNodeXML::load(const TCHAR* path)
{
  NarrowPathName buf(path);
  XMLNode *child = XMLNode::openFileHelper(buf);
  if (child == NULL)
    return NULL;

  DataNodeXML *data_node = new DataNodeXML(*child);
  delete child;
  return data_node;
}

DataNodeXML*
DataNodeXML::createRoot(const TCHAR *node_name)
{
  XMLNode new_root = XMLNode::createRoot(_tcsdup(node_name));
  return new DataNodeXML(new_root);
}

void
DataNodeXML::serialise(TextWriter &writer)
{
  m_xml_node->serialise(writer, true);
}

const TCHAR *
DataNodeXML::get_name() const
{
  return m_xml_node->getName();
}

DataNode*
DataNodeXML::add_child(const TCHAR *name)
{
  return new DataNodeXML(m_xml_node->AddChild(_tcsdup(name), false));
}

DataNode *
DataNodeXML::GetChildNamed(const TCHAR *name) const
{
  const XMLNode *child = m_xml_node->getChildNode(name, 0u);
  if (child == NULL)
    return NULL;

  return new DataNodeXML(*child);
}

DataNode::List
DataNodeXML::ListChildren() const
{
  List list;
  for (XMLNode::const_iterator i = m_xml_node->begin(),
         end = m_xml_node->end(); i != end; ++i)
    list.push_back(new DataNodeXML(*i));
  return list;
}

DataNode::List
DataNodeXML::ListChildrenNamed(const TCHAR *name) const
{
  List list;
  for (XMLNode::const_iterator i = m_xml_node->begin(),
         end = m_xml_node->end(); i != end; ++i)
    if (_tcsicmp(i->getName(), name) == 0)
      list.push_back(new DataNodeXML(*i));
  return list;
}

void
DataNodeXML::set_attribute(const TCHAR *name, const TCHAR *value)
{
  m_xml_node->AddAttribute(_tcsdup(name), _tcsdup(value));
}

bool
DataNodeXML::get_attribute(const TCHAR *name, tstring &value) const
{
  const TCHAR *v = m_xml_node->getAttribute(name);
  if (v == NULL)
    return false;

  tstring val(v);
  value = val;
  return true;
}

bool
DataNodeXML::save(const TCHAR* path)
{
  /// @todo make xml writing portable (unicode etc)
  TextWriter writer(path);
  if (writer.error())
    return false;

  serialise(writer);
  return true;
}
