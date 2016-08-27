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
#include "IO/BufferedOutputStream.hxx"
#include "Util/CharUtil.hpp"

#define INDENTCHAR '\t'

static void
WriteXMLChar(BufferedOutputStream &os, TCHAR ch)
{
  switch (ch) {
  case '<':
    os.Write("&lt;");
    break;
  case '>':
    os.Write("&gt;");
    break;
  case '&':
    os.Write("&amp;");
    break;
  case '\'':
    os.Write("&apos;");
    break;
  case '"':
    os.Write("&quot;");
    break;
  default:
    if (IsWhitespaceOrNull(ch))
      ch = ' ';

    os.Write(ch);
    break;
  }
}

static void
WriteXMLString(BufferedOutputStream &os, const tstring &source)
{
  for (auto ch : source)
    WriteXMLChar(os, ch);
}

static void
WriteIndent(BufferedOutputStream &os, unsigned n)
{
  while (n-- > 0)
    os.Write(INDENTCHAR);
}

void
XMLNode::Serialise(const Data &data, BufferedOutputStream &os, int format)
{
  bool has_children = false;

  // If the element has no name then assume this is the head node.
  if (!data.name.empty()) {
    // "<elementname "
    const unsigned cb = format == -1 ? 0 : format;

    WriteIndent(os, cb);
    os.Write('<');
    if (data.is_declaration)
      os.Write('?');
    os.Write(data.name.c_str());

    // Enumerate attributes and add them to the string
    for (auto i = data.attributes.begin(), end = data.attributes.end();
         i != end; ++i) {
      const Data::Attribute *pAttr = &*i;
      os.Write(' ');
      os.Write(pAttr->name.c_str());
      os.Write('=');
      os.Write('"');
      WriteXMLString(os, pAttr->value);
      os.Write('"');
      pAttr++;
    }

    has_children = data.HasChildren();
    if (data.is_declaration) {
      os.Write('?');
      os.Write('>');
      if (format != -1)
        os.Write('\n');
    } else
    // If there are child nodes we need to terminate the start tag
    if (has_children) {
      os.Write('>');
      if (format != -1)
        os.Write('\n');
    }
  }

  // Calculate the child format for when we recurse.  This is used to
  // determine the number of spaces used for prefixes.
  int child_format = -1;
  if (format != -1) {
    if (!data.name.empty())
      child_format = format + 1;
    else
      child_format = format;
  }

  /* write the child elements */
  for (auto i = data.begin(), end = data.end(); i != end; ++i)
    Serialise(*i->d, os, child_format);

  /* write the text */
  if (!data.text.empty()) {
    if (format != -1) {
      WriteIndent(os, format + 1);
      WriteXMLString(os, data.text);
      os.Write('\n');
    } else {
      WriteXMLString(os, data.text);
    }
  }

  if (!data.name.empty() && !data.is_declaration) {
    // If we have child entries we need to use long XML notation for
    // closing the element - "<elementname>blah blah blah</elementname>"
    if (has_children) {
      // "</elementname>\0"
      if (format != -1)
        WriteIndent(os, format);

      os.Write("</");
      os.Write(data.name.c_str());

      os.Write('>');
    } else {
      // If there are no children we can use shorthand XML notation -
      // "<elementname/>"
      // "/>\0"
      os.Write("/>");
    }

    if (format != -1)
      os.Write('\n');
  }
}

void
XMLNode::Serialise(BufferedOutputStream &os, bool format) const
{
  Serialise(*d, os, format ? 0 : -1);
}
