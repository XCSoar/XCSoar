// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataNodeXML.hpp"
#include "Node.hpp"
#include "util/StringAPI.hxx"

const TCHAR *
ConstDataNodeXML::GetName() const noexcept
{
  return node.GetName();
}

std::unique_ptr<WritableDataNode>
WritableDataNodeXML::AppendChild(const TCHAR *name) noexcept
{
  return std::make_unique<WritableDataNodeXML>(node.AddChild(name, false));
}

std::unique_ptr<ConstDataNode>
ConstDataNodeXML::GetChildNamed(const TCHAR *name) const noexcept
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
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    list.emplace_back(new ConstDataNodeXML(*i));
  return list;
}

ConstDataNode::List
ConstDataNodeXML::ListChildrenNamed(const TCHAR *name) const noexcept
{
  List list;
  for (auto i = node.begin(), end = node.end(); i != end; ++i)
    if (StringIsEqualIgnoreCase(i->GetName(), name))
      list.emplace_back(new ConstDataNodeXML(*i));
  return list;
}

void
WritableDataNodeXML::SetAttribute(const TCHAR *name, const TCHAR *value) noexcept
{
  node.AddAttribute(name, value);
}

const TCHAR *
ConstDataNodeXML::GetAttribute(const TCHAR *name) const noexcept
{
  return node.GetAttribute(name);
}
