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
#include <stddef.h>

struct XMLNode;

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

  // If the XML document is corrupted:
  //   - The "openFileHelper" method will stop execution and display an error message.
  //   - The 2 other methods will initialize the "pResults" variable with some information that
  //     can be used to trace the error.
  // you can have a detailed explanation of the parsing error with this function:

  extern bool global_error;

  XMLNode *ParseString(const TCHAR *xml_string, Results *pResults=NULL);
  XMLNode *ParseFile(const TCHAR *path, Results *pResults=NULL);
  XMLNode *OpenFileHelper(const TCHAR *path);

  /**
   * Parse XML errors into a user friendly string.
   */
  gcc_const
  const TCHAR *GetErrorMessage(Error error);
}

#endif
