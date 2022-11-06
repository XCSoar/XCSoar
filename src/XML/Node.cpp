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

#include <cassert>

XMLNode
XMLNode::CreateRoot(const TCHAR *name)
{
  return XMLNode(name, false);
}

XMLNode::XMLNode(tstring_view name,
                 bool is_declaration) noexcept
  :d(new Data(name, is_declaration))
{
  assert(d);
}

XMLNode &
XMLNode::AddChild(const tstring_view name,
                  bool is_declaration) noexcept
{
  d->children.push_back(XMLNode(name, is_declaration));
  return d->children.back();
}

void
XMLNode::AddText(tstring_view value) noexcept
{
  d->text.append(value);
}

const XMLNode *
XMLNode::GetChildNode(const TCHAR *name) const
{
  if (!d)
    return nullptr;

  for (auto i = d->begin(), end = d->end(); i != end; ++i) {
    const XMLNode &node = *i;
    if (StringIsEqualIgnoreCase(node.d->name.c_str(), name))
      return &node;
  }

  return nullptr;
}

const TCHAR *
XMLNode::GetAttribute(const TCHAR *name) const
{
  if (!d)
    return nullptr;

  for (auto i = d->attributes.begin(), end = d->attributes.end();
       i != end; ++i)
    if (StringIsEqualIgnoreCase(i->name.c_str(), name))
      return i->value.c_str();

  return nullptr;
}
