/**
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

#ifndef XCSOAR_XML_PARSER_HPP
#define XCSOAR_XML_PARSER_HPP

#include "Compiler.h"

#include <tchar.h>

class XMLNode;
class Path;

namespace XML {
  /** Enumeration for XML parse errors. */
  enum Error {
    eXMLErrorNone = 0,
    eXMLErrorEmpty,
    eXMLErrorFirstNotStartTag,
    eXMLErrorMissingTagName,
    eXMLErrorMissingEndTagName,
    eXMLErrorNoMatchingQuote,
    eXMLErrorUnmatchedEndTag,
    eXMLErrorUnexpectedToken,
    eXMLErrorInvalidTag,
    eXMLErrorNoElements,
    eXMLErrorFileNotFound
  };

  /** Structure used to obtain error details if the parse fails. */
  struct Results {
    enum Error error;
    unsigned line, column;
  };

  XMLNode *ParseString(const TCHAR *xml_string, Results *pResults=nullptr);
  XMLNode *ParseFile(Path path, Results *pResults=nullptr);

  /**
   * Parse XML errors into a user friendly string.
   */
  gcc_const
  const TCHAR *GetErrorMessage(Error error);
}

#endif
