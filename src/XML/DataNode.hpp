/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef DATANODE_HPP
#define DATANODE_HPP

#include "util/Compiler.h"

#include <list>
#include <memory>

#include <tchar.h>

class Angle;
class TextWriter;
class RoughTime;
class RoughTimeSpan;

/**
 * Class used as generic node for tree-structured data.
 * 
 */
class ConstDataNode {
public:
  using List = std::list<std::unique_ptr<ConstDataNode>>;

  ConstDataNode() noexcept = default;

  ConstDataNode(const ConstDataNode &) = delete;
  ConstDataNode &operator=(const ConstDataNode &) = delete;

  virtual ~ConstDataNode() noexcept;

  /**
   * Retrieve name of this node
   *
   * @return Copy of name
   */
  virtual const TCHAR *GetName() const noexcept = 0;

  /**
   * Retrieve child by name
   *
   * @param name Name of child
   *
   * @return Pointer to child if found, or nullptr
   */
  virtual std::unique_ptr<ConstDataNode> GetChildNamed(const TCHAR *name) const noexcept = 0;

  /**
   * Obtains a list of all children.  The caller is responsible for
   * deleting the elements.
   */
  virtual List ListChildren() const noexcept = 0;

  /**
   * Obtains a list of all children matching the specified name.
   * Returns an empty list if there is no such child.  The caller is
   * responsible for deleting the elements.
   */
  virtual List ListChildrenNamed(const TCHAR *name) const noexcept = 0;

  /**
   * Retrieve named attribute value
   *
   * @param name Name of attribute
   *
   * @return the value or nullptr if it does not exist
   */
  virtual const TCHAR *GetAttribute(const TCHAR *name) const noexcept = 0;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const TCHAR *name, double &value) const noexcept;

  bool GetAttribute(const TCHAR *name, Angle &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const TCHAR *name, int &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const TCHAR *name, unsigned &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const TCHAR *name, bool &value) const noexcept;

  gcc_pure
  RoughTime GetAttributeRoughTime(const TCHAR *name) const noexcept;

  gcc_pure
  RoughTimeSpan GetAttributeRoughTimeSpan(const TCHAR *start_name,
                                          const TCHAR *end_name) const noexcept;
};

/**
 * Class used as generic node for tree-structured data.
 */
class WritableDataNode {
public:
  WritableDataNode() noexcept = default;

  WritableDataNode(const WritableDataNode &) = delete;
  WritableDataNode &operator=(const WritableDataNode &) = delete;

  virtual ~WritableDataNode() noexcept;

  /**
   * Add child to this node
   *
   * @param name Name of child
   *
   * @return Pointer to new child
   */
  virtual std::unique_ptr<WritableDataNode> AppendChild(const TCHAR *name) noexcept = 0;

  /**
   * Set named attribute value
   *
   * @param name Name of attribute
   * @param value Value of attribute
   */
  virtual void SetAttribute(const TCHAR *name, const TCHAR *value) noexcept = 0;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (double)
   */
  void SetAttribute(const TCHAR *name, double value) noexcept;

  void SetAttribute(const TCHAR *name, Angle value) noexcept;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (int)
   */
  void SetAttribute(const TCHAR *name, int value) noexcept;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (unsigned int)
   */
  void SetAttribute(const TCHAR *name, unsigned value) noexcept;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (boolean)
   */
  void SetAttribute(const TCHAR *name, bool value) noexcept;

  /**
   * Set named attribute value.  No-op if the #RoughTime object is
   * invalid.
   */
  void SetAttribute(const TCHAR *name, RoughTime value) noexcept;
};

#endif
