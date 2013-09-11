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

#ifndef DATANODE_XML_HPP
#define DATANODE_XML_HPP

#include "DataNode.hpp"
#include "XML/Node.hpp"

/**
 * DataNode implementation for XML files
 */
class DataNodeXML:
  public DataNode
{
  XMLNode node;

protected:
  /** 
   * Construct a node from an XMLNode
   * 
   * @param the_node XML node reflecting this node
   * 
   * @return Initialised object
   */
  explicit DataNodeXML(const XMLNode &_node)
    :node(_node) {}

  explicit DataNodeXML(const XMLNode &&_node)
    :node(std::move(_node)) {}

public:
  DataNodeXML(DataNodeXML &&other)
    :node(std::move(other.node)) {
  }

  /**
   * Create a DataNode tree from an XML file
   *
   * @param path Path to file to load
   *
   * @return Root node (or NULL on failure)
   */
  static DataNode *Load(const TCHAR* path);

  /**
   * Create root node
   *
   * @param node_name Name of root node
   *
   * @return Pointer to root node
   */
  gcc_pure
  static DataNodeXML CreateRoot(const TCHAR *node_name) {
    return DataNodeXML(XMLNode::CreateRoot(node_name));
  }

  /**
   * Save tree canonically to file
   *
   * @param path Path of file to save to
   *
   * @return True on success
   */
  bool Save(const TCHAR* path);

  /* virtual methods from DataNode */
  virtual const TCHAR *GetName() const;
  virtual DataNode *AppendChild(const TCHAR *name);
  virtual DataNode *GetChildNamed(const TCHAR *name) const;
  virtual List ListChildren() const;
  virtual List ListChildrenNamed(const TCHAR *name) const;
  virtual void Serialise(TextWriter &writer) const;
  virtual void SetAttribute(const TCHAR *name, const TCHAR *value);
  virtual const TCHAR *GetAttribute(const TCHAR *name) const;
};

#endif
