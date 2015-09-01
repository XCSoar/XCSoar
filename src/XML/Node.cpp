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
#include "Util/StringAPI.hxx"

#include <assert.h>

XMLNode
XMLNode::CreateRoot(const TCHAR *name)
{
  return XMLNode(name, false);
}

XMLNode::XMLNode(const TCHAR *name, bool is_declaration)
  :d(new Data(name, is_declaration))
{
  assert(d);
}

XMLNode::XMLNode(const TCHAR *name, size_t name_length, bool is_declaration)
  :d(new Data(name, name_length, is_declaration))
{
  assert(d);
}

XMLNode &
XMLNode::AddChild(const TCHAR *name, bool is_declaration)
{
  assert(name != nullptr);

  d->children.push_back(XMLNode(name, is_declaration));
  return d->children.back();
}

XMLNode &
XMLNode::AddChild(const TCHAR *name, size_t name_length, bool is_declaration)
{
  assert(name != nullptr);

  d->children.push_back(XMLNode(name, name_length, is_declaration));
  return d->children.back();
}

void
XMLNode::AddText(const TCHAR *value)
{
  assert(value != nullptr);

  d->text.append(value);
}

void
XMLNode::AddText(const TCHAR *text, size_t length)
{
  assert(text != nullptr);

  d->text.append(text, length);
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
