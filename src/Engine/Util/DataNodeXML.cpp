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

#include "DataNodeXML.hpp"
#include "OS/PathName.hpp"
#include "xmlParser.h"

#include <stdio.h>

static LPTSTR stringDup(const tstring text) 
{
  return stringDup(text.c_str());
}

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
  XMLNode child = XMLNode::openFileHelper(buf);
  if (child.isEmpty())
    return NULL;

  return new DataNodeXML(child);
}

DataNodeXML*
DataNodeXML::createRoot(const tstring &node_name)
{
  XMLNode new_root = XMLNode::createRoot(stringDup(node_name));
  return new DataNodeXML(new_root);
}

const tstring
DataNodeXML::serialise()
{
  LPTSTR text = m_xml_node->createXMLString(1, NULL);
  return tstring(text);
}

const tstring
DataNodeXML::get_name() const
{
  return tstring(m_xml_node->getName());
}

DataNode*
DataNodeXML::add_child(const tstring &name)
{
  return new DataNodeXML(m_xml_node->AddChild(stringDup(name), false));
}

DataNode*
DataNodeXML::get_child_by_name(const tstring name, const unsigned i) const
{
  XMLNode child = m_xml_node->getChildNode(name.c_str(), i);
  if (child.isEmpty())
    return NULL;

  return new DataNodeXML(child);
}

DataNode*
DataNodeXML::get_child(unsigned i) const
{
  XMLNode child = m_xml_node->getChildNode(i);
  if (child.isEmpty())
    return NULL;

  return new DataNodeXML(child);
}

void
DataNodeXML::set_attribute(const tstring &name, const tstring value)
{
  m_xml_node->AddAttribute(stringDup(name), stringDup(value));
}

bool
DataNodeXML::get_attribute(const tstring &name, tstring &value) const
{
  const TCHAR *v = m_xml_node->getAttribute(name.c_str());
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
  NarrowPathName buf(path);
  FILE* file = fopen(buf, "wt");
  if (file == NULL)
    return false;

  const TCHAR* text = serialise().c_str();

#ifdef _UNICODE
  char text2[_tcslen(text) * 4];
  ::WideCharToMultiByte(CP_ACP, 0, text, -1, text2, sizeof(text2), NULL, NULL);
  fprintf(file, "%s", text2);
#else
  fprintf(file, "%s", text);
#endif

  fclose(file);

  return true;
}
