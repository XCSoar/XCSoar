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
  const TCHAR *GetName() const noexcept override;
  std::unique_ptr<ConstDataNode> GetChildNamed(const TCHAR *name) const noexcept override;
  List ListChildren() const noexcept override;
  List ListChildrenNamed(const TCHAR *name) const noexcept override;
  const TCHAR *GetAttribute(const TCHAR *name) const noexcept override;
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
  std::unique_ptr<WritableDataNode> AppendChild(const TCHAR *name) noexcept override;
  void SetAttribute(const TCHAR *name, const TCHAR *value) noexcept override;
};
