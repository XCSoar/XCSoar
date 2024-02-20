// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"

#include <list>
#include <memory>

class Angle;
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
  virtual const char *GetName() const noexcept = 0;

  /**
   * Retrieve child by name
   *
   * @param name Name of child
   *
   * @return Pointer to child if found, or nullptr
   */
  virtual std::unique_ptr<ConstDataNode> GetChildNamed(const char *name) const noexcept = 0;

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
  virtual List ListChildrenNamed(const char *name) const noexcept = 0;

  /**
   * Retrieve named attribute value
   *
   * @param name Name of attribute
   *
   * @return the value or nullptr if it does not exist
   */
  virtual const char *GetAttribute(const char *name) const noexcept = 0;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const char *name, double &value) const noexcept;

  bool GetAttribute(const char *name, Angle &value) const noexcept;

  bool GetAttribute(const char *name, FloatDuration &value) const noexcept;
  bool GetAttribute(const char *name,
                    std::chrono::duration<unsigned> &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const char *name, int &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const char *name, unsigned &value) const noexcept;

  /**
   * Retrieve named attribute value, with numeric conversion
   *
   * @param name Name of attribute
   * @param value Value (written)
   *
   * @return True if attribute exists
   */
  bool GetAttribute(const char *name, bool &value) const noexcept;

  [[gnu::pure]]
  RoughTime GetAttributeRoughTime(const char *name) const noexcept;

  [[gnu::pure]]
  RoughTimeSpan GetAttributeRoughTimeSpan(const char *start_name,
                                          const char *end_name) const noexcept;
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
  virtual std::unique_ptr<WritableDataNode> AppendChild(const char *name) noexcept = 0;

  /**
   * Set named attribute value
   *
   * @param name Name of attribute
   * @param value Value of attribute
   */
  virtual void SetAttribute(const char *name, const char *value) noexcept = 0;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (double)
   */
  void SetAttribute(const char *name, double value) noexcept;

  void SetAttribute(const char *name, Angle value) noexcept;

  void SetAttribute(const char *name, FloatDuration value) noexcept {
    SetAttribute(name, value.count());
  }

  void SetAttribute(const char *name,
                    std::chrono::duration<unsigned> value) noexcept {
    SetAttribute(name, value.count());
  }

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (int)
   */
  void SetAttribute(const char *name, int value) noexcept;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (unsigned int)
   */
  void SetAttribute(const char *name, unsigned value) noexcept;

  /**
   * Set named attribute value, with numeric to text conversion
   *
   * @param name Name of attribute
   * @param value Value (boolean)
   */
  void SetAttribute(const char *name, bool value) noexcept;

  /**
   * Set named attribute value.  No-op if the #RoughTime object is
   * invalid.
   */
  void SetAttribute(const char *name, RoughTime value) noexcept;

  /* just here to prevent implicit pointer-to-bool casts
     (e.g. TCHAR/wchar_t strings) */
  void SetAttribute(const char *name, const auto *value) noexcept = delete;
};
