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

#ifndef DATANODE_XML_HPP
#define DATANODE_XML_HPP

#include "DataNode.hpp"

struct XMLNode;

/**
 * DataNode implementation for XML files
 */
class DataNodeXML:
  public DataNode
{
protected:
  /** 
   * Construct a node from an XMLNode
   * 
   * @param the_node XML node reflecting this node
   * 
   * @return Initialised object
   */
  explicit DataNodeXML(const XMLNode& the_node);

public:
  ~DataNodeXML();

  /**
   * Create a DataNode tree from an XML file
   *
   * @param path Path to file to load
   *
   * @return Root node (or NULL on failure)
   */
  static DataNode* load(const TCHAR* path);

  /**
   * Create root node
   *
   * @param node_name Name of root node
   *
   * @return Pointer to root node
   */
  static DataNodeXML* createRoot(const tstring &node_name);

  const tstring serialise();

  /**
   * Save tree canonically to file
   *
   * @param path Path of file to save to
   *
   * @return True on success
   */
  bool save(const TCHAR* path);

  const tstring get_name() const;

  DataNode* add_child(const tstring &name);
  DataNode* get_child(unsigned i) const;
  DataNode* get_child_by_name(const tstring name, const unsigned i=0) const;

  void set_attribute(const tstring &name, const tstring value);
  bool get_attribute(const tstring &name, tstring &value) const;

private:
  XMLNode *m_xml_node;
};

#endif
