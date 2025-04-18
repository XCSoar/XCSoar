// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  explicit ConstDataNodeXML(const XMLNode &_node) noexcept
    :node(_node) {}

  /* virtual methods from ConstDataNode */
  const char *GetName() const noexcept override;
  std::unique_ptr<ConstDataNode> GetChildNamed(const char *name) const noexcept override;
  List ListChildren() const noexcept override;
  List ListChildrenNamed(const char *name) const noexcept override;
  const char *GetAttribute(const char *name) const noexcept override;
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
  explicit WritableDataNodeXML(XMLNode &_node) noexcept
    :node(_node) {}

  /* virtual methods from WritableDataNode */
  std::unique_ptr<WritableDataNode> AppendChild(const char *name) noexcept override;
  void SetAttribute(const char *name, const char *value) noexcept override;
};
