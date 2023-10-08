/*
 ****************************************************************************
 * <P> XML.c - implementation file for basic XML parser written in ANSI C++
 * for portability. It works by using recursion and a node tree for breaking
 * down the elements of an XML document.  </P>
 *
 * @version     V1.08
 *
 * @author      Frank Vanden Berghen
 * based on original implementation by Martyn C Brown
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************
 */

#include "Node.hpp"
#include "util/StringAPI.hxx"

XMLNode
XMLNode::CreateRoot(const char *name) noexcept
{
  return XMLNode(name, false);
}

XMLNode::XMLNode(std::string_view _name,
                 bool _is_declaration) noexcept
  :name(_name),
   is_declaration(_is_declaration)
{
}

XMLNode &
XMLNode::AddChild(const std::string_view _name,
                  bool _is_declaration) noexcept
{
  children.push_back(XMLNode(_name, _is_declaration));
  return children.back();
}

const XMLNode *
XMLNode::GetChildNode(const char *name) const noexcept
{
  for (const auto &i : children) {
    if (StringIsEqualIgnoreCase(i.GetName(), name))
      return &i;
  }

  return nullptr;
}

const char *
XMLNode::GetAttribute(const char *name) const noexcept
{
  for (const auto &i : attributes)
    if (StringIsEqualIgnoreCase(i.name.c_str(), name))
      return i.value.c_str();

  return nullptr;
}
