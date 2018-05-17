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

#ifndef DATANODE_XML_HPP
#define DATANODE_XML_HPP

#include "DataNode.hpp"

class XMLNode;

/**
 * ConstDataNode implementation for XML files
 */
class ConstDataNodeXML final : public ConstDataNode {
  const XMLNode &node;

public:
  /**
   * Construct a node from an XMLNode
   *
   * @param the_node XML node reflecting this node
   *
   * @return Initialised object
   */
  explicit ConstDataNodeXML(const XMLNode &_node)
    :node(_node) {}

  /* virtual methods from ConstDataNode */
  const TCHAR *GetName() const override;
  ConstDataNode *GetChildNamed(const TCHAR *name) const override;
  List ListChildren() const override;
  List ListChildrenNamed(const TCHAR *name) const override;
  const TCHAR *GetAttribute(const TCHAR *name) const override;
};

/**
 * WritableDataNode implementation for XML files
 */
class WritableDataNodeXML final : public WritableDataNode {
  XMLNode &node;

public:
  /**
   * Construct a node from an XMLNode
   *
   * @param the_node XML node reflecting this node
   *
   * @return Initialised object
   */
  explicit WritableDataNodeXML(XMLNode &_node)
    :node(_node) {}

  /* virtual methods from WritableDataNode */
  WritableDataNode *AppendChild(const TCHAR *name) override;
  void SetAttribute(const TCHAR *name, const TCHAR *value) override;
};

#endif
