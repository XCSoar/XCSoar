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
 * NOTE:
 *
 *   If you add "#define APPROXIMATE_PARSING", on the first line of this file
 *   the parser will see the following XML-stream:
 *     <data name="n1">
 *     <data name="n2">
 *     <data name="n3" />
 *   as equivalent to the following XML-stream:
 *     <data name="n1" />
 *     <data name="n2" />
 *     <data name="n3" />
 *   This can be useful for badly-formed XML-streams but prevent the use
 *   of the following XML-stream:
 *     <data name="n1">
 *        <data name="n2">
 *            <data name="n3" />
 *        </data>
 *     </data>
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
#include "IO/TextWriter.hpp"
#include "Util/StringUtil.hpp"

#include <assert.h>

#define INDENTCHAR '\t'

static void
WriteXMLString(TextWriter &writer, const TCHAR *source)
{
  while (*source) {
    switch (*source) {
    case '<':
      writer.write("&lt;");
      break;
    case '>':
      writer.write("&gt;");
      break;
    case '&':
      writer.write("&amp;");
      break;
    case '\'':
      writer.write("&apos;");
      break;
    case '"':
      writer.write("&quot;");
      break;
    default:
      writer.write(*source);
      break;
    }
    source++;
  }
}

static void
WriteIndent(TextWriter &writer, unsigned n)
{
  while (n-- > 0)
    writer.write(INDENTCHAR);
}

void
XMLNode::Serialise(const Data &data, TextWriter &writer, int format)
{
  bool has_children = false;

  // If the element has no name then assume this is the head node.
  if (!data.name.empty()) {
    // "<elementname "
    const unsigned cb = format == -1 ? 0 : format;

    WriteIndent(writer, cb);
    writer.write('<');
    if (data.is_declaration)
      writer.write('?');
    writer.write(data.name.c_str());

    // Enumerate attributes and add them to the string
    for (auto i = data.attributes.begin(), end = data.attributes.end();
         i != end; ++i) {
      const Data::Attribute *pAttr = &*i;
      writer.write(' ');
      writer.write(pAttr->name.c_str());
      writer.write('=');
      writer.write('"');
      WriteXMLString(writer, pAttr->value.c_str());
      writer.write('"');
      pAttr++;
    }

    has_children = data.HasChildren();
    if (data.is_declaration) {
      writer.write('?');
      writer.write('>');
      if (format != -1)
        writer.newline();
    } else
    // If there are child nodes we need to terminate the start tag
    if (has_children) {
      writer.write('>');
      if (format != -1)
        writer.newline();
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
    Serialise(*i->d, writer, child_format);

  /* write the text */
  if (!data.text.empty()) {
    if (format != -1) {
      WriteIndent(writer, format + 1);
      WriteXMLString(writer, data.text.c_str());
      writer.newline();
    } else {
      WriteXMLString(writer, data.text.c_str());
    }
  }

  if (!data.name.empty() && !data.is_declaration) {
    // If we have child entries we need to use long XML notation for
    // closing the element - "<elementname>blah blah blah</elementname>"
    if (has_children) {
      // "</elementname>\0"
      if (format != -1)
        WriteIndent(writer, format);

      writer.write("</");
      writer.write(data.name.c_str());

      writer.write('>');
    } else {
      // If there are no children we can use shorthand XML notation -
      // "<elementname/>"
      // "/>\0"
      writer.write("/>");
    }

    if (format != -1)
      writer.newline();
  }
}

void
XMLNode::Serialise(TextWriter &writer, bool format) const
{
  Serialise(*d, writer, format ? 0 : -1);
}
