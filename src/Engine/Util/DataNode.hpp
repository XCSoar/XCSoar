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

#ifndef DATANODE_HPP
#define DATANODE_HPP

#include "tstring.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Util/NonCopyable.hpp"

#include <list>

class TextWriter;

/**
 * Class used as generic node for tree-structured data.
 * 
 */
class DataNode : private NonCopyable
{
public:
  typedef std::list<DataNode *> List;

  virtual ~DataNode();

  /**
   * Retrieve name of this node
   *
   * @return Copy of name
   */
  virtual const TCHAR *get_name() const = 0;

  /**
   * Add child to this node
   *
   * @param name Name of child
   *
   * @return Pointer to new child
   */
  virtual DataNode* add_child(const TCHAR *name) = 0;

  /**
   * Retrieve child by name
   *
   * @param name Name of child
   *
   * @return Pointer to child if found, or NULL
   */
  virtual DataNode *GetChildNamed(const TCHAR *name) const = 0;

  /**
   * Obtains a list of all children.  The caller is responsible for
   * deleting the elements.
   */
  virtual List ListChildren() const = 0;

  /**
   * Obtains a list of all children matching the specified name.
   * Returns an empty list if there is no such child.  The caller is
   * responsible for deleting the elements.
   */
  virtual List ListChildrenNamed(const TCHAR *name) const = 0;

  /**
   * Writes the canonical serialised form of this node to a
   * TextWriter.
   *
   * @param writer the destination file
   */
  virtual void serialise(TextWriter &writer) = 0;

  /**
   * Set named attribute value
   *
   * @param name Name of attribute
   * @param value Value of attribute
   */
  virtual void set_attribute(const TCHAR *name, const TCHAR *value) = 0;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (fixed)
   */
  void set_attribute(const TCHAR *name, fixed value);

  void set_attribute(const TCHAR *name, Angle value);

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (int)
   */
  void set_attribute(const TCHAR *name, int value);

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (unsigned int)
   */
  void set_attribute(const TCHAR *name, unsigned value);

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (boolean)
   */
  void set_attribute(const TCHAR *name, bool &value);

  /**
   * Retrieve named attribute value
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  virtual bool get_attribute(const TCHAR *name, tstring &value) const = 0;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool get_attribute(const TCHAR *name, fixed &value) const;

  bool get_attribute(const TCHAR *name, Angle &value) const;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool get_attribute(const TCHAR *name, int &value) const;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool get_attribute(const TCHAR *name, unsigned &value) const;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool get_attribute(const TCHAR *name, bool &value) const;
};

#endif
