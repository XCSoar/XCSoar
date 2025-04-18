// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataNodeXML.hpp"
#include "Node.hpp"
#include "util/StringAPI.hxx"

const char *
ConstDataNodeXML::GetName() const noexcept
{
  return node.GetName();
}

std::unique_ptr<WritableDataNode>
WritableDataNodeXML::AppendChild(const char *name) noexcept
{
  return std::make_unique<WritableDataNodeXML>(node.AddChild(name, false));
}

std::unique_ptr<ConstDataNode>
ConstDataNodeXML::GetChildNamed(const char *name) const noexcept
{
  const XMLNode *child = node.GetChildNode(name);
  if (child == nullptr)
    return nullptr;

  return std::make_unique<ConstDataNodeXML>(*child);
}

ConstDataNode::List
ConstDataNodeXML::ListChildren() const noexcept
{
  List list;
  for (const auto &i : node)
    list.emplace_back(new ConstDataNodeXML(i));
  return list;
}

ConstDataNode::List
ConstDataNodeXML::ListChildrenNamed(const char *name) const noexcept
{
  List list;
  for (const auto &i : node)
    if (StringIsEqualIgnoreCase(i.GetName(), name))
      list.emplace_back(new ConstDataNodeXML(i));
  return list;
}

void
WritableDataNodeXML::SetAttribute(const char *name, const char *value) noexcept
{
  node.AddAttribute(name, value);
}

const char *
ConstDataNodeXML::GetAttribute(const char *name) const noexcept
{
  return node.GetAttribute(name);
}
