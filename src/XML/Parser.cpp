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

#include "Parser.hpp"
#include "Node.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringUtil.hpp"
#include "Util/NumberParser.hpp"
#include "IO/FileLineReader.hpp"

#include <stdexcept>

#include <assert.h>

namespace XML {
  /** Main structure used for parsing XML. */
  struct Parser {
    const TCHAR *lpXML;
    unsigned nIndex;
    Error error;
    const TCHAR *lpEndTag;
    size_t cbEndTag;
    bool nFirst;
  };

  /** Enumeration used to decipher what type a token is. */
  enum TokenTypeTag {
    eTokenText = 0,
    eTokenQuotedText,
    eTokenTagStart,         /* "<"            */
    eTokenTagEnd,           /* "</"           */
    eTokenCloseTag,         /* ">"            */
    eTokenEquals,           /* "="            */
    eTokenDeclaration,      /* "<?"           */
    eTokenShortHandClose,   /* "/>"           */
    eTokenError
  };

  struct NextToken {
    const TCHAR *pStr;

    /**
     * The number of characters that have been read.
     */
    size_t length;

    TokenTypeTag type;
  };

  /** Enumeration used when parsing attributes. */
  enum Attrib {
    eAttribName = 0,
    eAttribEquals,
    eAttribValue
  };

  /**
   * Enumeration used when parsing elements to dictate whether we are
   * currently inside a tag.
   */
  enum Status {
    eInsideTag = 0,
    eOutsideTag
  };

  static XML::NextToken
  GetNextToken(Parser *pXML);

  static bool
  ParseXMLElement(XMLNode &node, Parser *pXML);
}

/**
 * This function is the opposite of the function "toXMLString". It
 * decodes the escape sequences &amp;, &quot;, &apos;, &lt;, &gt; and
 * replace them by the characters &,",',<,>. This function is used
 * internally by the XML Parser. All the calls to the XML library will
 * always gives you back "decoded" strings.
 *
 * @param ss string
 * @param lo length of string
 * @return new allocated string converted from xml
 */
static TCHAR *
FromXMLString(const TCHAR *ss, size_t lo)
{
  assert(ss != nullptr);

  const TCHAR *end = ss + lo;

  /* allocate a buffer with the size of the input string; we know for
     sure that this is enough, because resolving entities can only
     shrink the string, but never grows */
  TCHAR *d = (TCHAR *)malloc((lo + 1) * sizeof(*d));
  assert(d);
  TCHAR *result = d;
  while (ss < end && *ss) {
    if (*ss == _T('&')) {
      ss++;
      if (StringIsEqualIgnoreCase(ss, _T("lt;" ), 3)) {
        *(d++) = _T('<' );
        ss += 3;
      } else if (StringIsEqualIgnoreCase(ss, _T("gt;" ), 3)) {
        *(d++) = _T('>' );
        ss += 3;
      } else if (StringIsEqualIgnoreCase(ss, _T("amp;" ), 4)) {
        *(d++) = _T('&' );
        ss += 4;
      } else if (StringIsEqualIgnoreCase(ss, _T("apos;"), 5)) {
        *(d++) = _T('\'');
        ss += 5;
      } else if (StringIsEqualIgnoreCase(ss, _T("quot;"), 5)) {
        *(d++) = _T('"' );
        ss += 5;
      } else if (*ss == '#') {
        /* number entity */

        ++ss;

        TCHAR *endptr;
        unsigned i = ParseUnsigned(ss, &endptr, 10);
        if (endptr == ss || endptr >= end || *endptr != ';') {
          free(result);
          return nullptr;
        }

        // XXX convert to UTF-8 if !_UNICODE
        TCHAR ch = (TCHAR)i;
        if (ch == 0)
          ch = ' ';

        *d++ = ch;
        ss = endptr + 1;
      } else {
        free(result);
        return nullptr;
      }
    } else {
      *(d++) = *ss;
      ss++;
    }
  }
  *d = 0;

  /* shrink the memory allocation just in case we allocated too
     much */
  d = (TCHAR *)realloc(result, (d + 1 - result) * sizeof(*d));
  if (d != nullptr)
    result = d;

  return result;
}

gcc_pure
static bool
CompareTagName(const TCHAR *cclose, const TCHAR *copen)
{
  assert(cclose != nullptr);
  assert(copen != nullptr);

  size_t l = _tcslen(cclose);
  if (!StringIsEqualIgnoreCase(cclose, copen, l))
    return false;

  const TCHAR c = copen[l];
  if (IsWhitespaceOrNull(c) ||
      (c == _T('/')) ||
      (c == _T('<')) ||
      (c == _T('>')) ||
      (c == _T('=')))
    return true;

  return false;
}

/**
 * Obtain the next character from the string.
 */
static inline TCHAR
GetNextChar(XML::Parser *pXML)
{
  TCHAR ch = pXML->lpXML[pXML->nIndex];
  if (ch != 0)
    pXML->nIndex++;
  return ch;
}

/**
 * Find next non-white space character.
 */
static TCHAR
FindNonWhiteSpace(XML::Parser *pXML)
{
  assert(pXML);

    // Iterate through characters in the string until we find a NULL or a
  // non-white space character
  TCHAR ch;
  while ((ch = GetNextChar(pXML)) != 0) {
    if (!IsWhitespaceOrNull(ch))
      return ch;
  }
  return 0;
}

/**
 * Find the next token in a string.
 */
static XML::NextToken
XML::GetNextToken(Parser *pXML)
{
  XML::NextToken result;
  const TCHAR *lpXML;
  TCHAR ch;
  TCHAR temp_ch;
  size_t size;
  unsigned n;
  bool found_match;
  bool is_text = false;

  // Find next non-white space character
  ch = FindNonWhiteSpace(pXML);
  if (gcc_unlikely(ch == 0)) {
    // If we failed to obtain a valid character
    return { nullptr, 0, eTokenError };
  }

  // Cache the current string pointer
  lpXML = pXML->lpXML;
  result.pStr = &lpXML[pXML->nIndex - 1];

  switch (ch) {
    // Check for quotes
  case _T('\''):
  case _T('\"'):
    // Type of token
    result.type = eTokenQuotedText;
    temp_ch = ch;
    n = pXML->nIndex;

    // Set the size
    size = 1;
    found_match = false;

    // Search through the string to find a matching quote
    while (((ch = GetNextChar(pXML))) != 0) {
      size++;
      if (ch == temp_ch) {
        found_match = true;
        break;
      }
      if (ch == _T('<'))
        break;
    }

    // If we failed to find a matching quote
    if (!found_match) {
      pXML->nIndex = n;
      is_text = true;
      break;
    }

    //  4.02.2002
    if (FindNonWhiteSpace(pXML)) {
      pXML->nIndex--;
    }

    break;

    // Equals (used with attribute values)
  case _T('='):
    size = 1;
    result.type = eTokenEquals;
    break;

    // Close tag
  case _T('>'):
    size = 1;
    result.type = eTokenCloseTag;
    break;

    // Check for tag start and tag end
  case _T('<'):

    // Peek at the next character to see if we have an end tag '</',
    // or an xml declaration '<?'
    temp_ch = pXML->lpXML[pXML->nIndex];

    // If we have a tag end...
    if (temp_ch == _T('/')) {
      // Set the type and ensure we point at the next character
      GetNextChar(pXML);
      result.type = eTokenTagEnd;
      size = 2;
    }

    // If we have an XML declaration tag
    else if (temp_ch == _T('?')) {

      // Set the type and ensure we point at the next character
      GetNextChar(pXML);
      result.type = eTokenDeclaration;
      size = 2;
    }

    // Otherwise we must have a start tag
    else {
      result.type = eTokenTagStart;
      size = 1;
    }
    break;

    // Check to see if we have a short hand type end tag ('/>').
  case _T('/'):

    // Peek at the next character to see if we have a short end tag '/>'
    temp_ch = pXML->lpXML[pXML->nIndex];

    // If we have a short hand end tag...
    if (temp_ch == _T('>')) {
      // Set the type and ensure we point at the next character
      GetNextChar(pXML);
      result.type = eTokenShortHandClose;
      size = 2;
      break;
    }

    // If we haven't found a short hand closing tag then drop into the
    // text process

#if GCC_CHECK_VERSION(7,0)
    [[fallthrough]];
#endif

    // Other characters
  default:
    is_text = true;
  }

  // If this is a TEXT node
  if (is_text) {
    // Indicate we are dealing with text
    result.type = eTokenText;
    size = 1;
    bool nExit = false;

    while (!nExit && ((ch = GetNextChar(pXML)) != 0)) {
      if (IsWhitespaceOrNull(ch))
        // Break when we find white space
        break;

      switch (ch) {
      // If we find a slash then this maybe text or a short hand end tag.
      case _T('/'):

        // Peek at the next character to see it we have short hand end tag
        temp_ch = pXML->lpXML[pXML->nIndex];

        // If we found a short hand end tag then we need to exit the loop
        if (temp_ch == _T('>')) {
          pXML->nIndex--; //  03.02.2002
          nExit = true;
        } else {
          size++;
        }
        break;

        // Break when we find a terminator and decrement the index and
        // column count so that we are pointing at the right character
        // the next time we are called.
      case _T('<'):
      case _T('>'):
      case _T('='):
        pXML->nIndex--;
      nExit = true;
      break;

      case 0:
        nExit = true;
        break;

      default:
        size++;
      }
    }
  }
  result.length = size;

  return result;
}

const TCHAR *
XML::GetErrorMessage(Error error)
{
  switch (error) {
  case eXMLErrorNone:
    return _T("No error");
  case eXMLErrorEmpty:
    return _T("No XML data");
  case eXMLErrorFirstNotStartTag:
    return _T("First token not start tag");
  case eXMLErrorMissingTagName:
    return _T("Missing start tag name");
  case eXMLErrorMissingEndTagName:
    return _T("Missing end tag name");
  case eXMLErrorNoMatchingQuote:
    return _T("Unmatched quote");
  case eXMLErrorUnmatchedEndTag:
    return _T("Unmatched end tag");
  case eXMLErrorUnexpectedToken:
    return _T("Unexpected token found");
  case eXMLErrorInvalidTag:
    return _T("Invalid tag found");
  case eXMLErrorNoElements:
    return _T("No elements found");
  case eXMLErrorFileNotFound:
    return _T("File not found");
  }

  return _T("Unknown");
}

/**
 * Recursively parse an XML element.
 */
static bool
XML::ParseXMLElement(XMLNode &node, Parser *pXML)
{
  bool is_declaration;
  const TCHAR *text = nullptr;
  XMLNode *pNew;
  enum Status status; // inside or outside a tag
  enum Attrib attrib = eAttribName;

  /* the name of the attribute that is currently being */
  tstring attribute_name;

  assert(pXML);

  // If this is the first call to the function
  if (pXML->nFirst) {
    // Assume we are outside of a tag definition
    pXML->nFirst = false;
    status = eOutsideTag;
  } else {
    // If this is not the first call then we should only be called when inside a tag.
    status = eInsideTag;
  }

  // Iterate through the tokens in the document
  while (true) {
    // Obtain the next token
    NextToken token = GetNextToken(pXML);
    if (gcc_unlikely(token.type == eTokenError))
      return false;

    // Check the current status
    switch (status) {
      // If we are outside of a tag definition
    case eOutsideTag:

      // Check what type of token we obtained
      switch (token.type) {
        // If we have found text or quoted text
      case eTokenText:
      case eTokenQuotedText:
      case eTokenEquals:
        if (text == nullptr)
          text = token.pStr;

        break;

        // If we found a start tag '<' and declarations '<?'
      case eTokenTagStart:
      case eTokenDeclaration:
        // Cache whether this new element is a declaration or not
        is_declaration = token.type == eTokenDeclaration;

        // If we have node text then add this to the element
        if (text != nullptr) {
          size_t length = StripRight(text, token.pStr - text);
          node.AddText(text, length);
          text = nullptr;
        }

        // Find the name of the tag
        token = GetNextToken(pXML);

        // Return an error if we couldn't obtain the next token or
        // it wasnt text
        if (token.type != eTokenText) {
          pXML->error = eXMLErrorMissingTagName;
          return false;
        }

        // If the name of the new element differs from the name of
        // the current element we need to add the new element to
        // the current one and recurse
        pNew = &node.AddChild(token.pStr, token.length,
                              is_declaration);

        while (true) {
          // Callself to process the new node.  If we return
          // FALSE this means we dont have any more
          // processing to do...

          if (!ParseXMLElement(*pNew, pXML)) {
            return false;
          } else {
            // If the call to recurse this function
            // evented in a end tag specified in XML then
            // we need to unwind the calls to this
            // function until we find the appropriate node
            // (the element name and end tag name must
            // match)
            if (pXML->cbEndTag) {
              // If we are back at the root node then we
              // have an unmatched end tag
              if (node.GetName() == nullptr) {
                pXML->error = eXMLErrorUnmatchedEndTag;
                return false;
              }

              // If the end tag matches the name of this
              // element then we only need to unwind
              // once more...

              if (CompareTagName(node.GetName(), pXML->lpEndTag)) {
                pXML->cbEndTag = 0;
              }

              return true;
            } else {
              // If we didn't have a new element to create
              break;
            }
          }
        }
        break;

        // If we found an end tag
      case eTokenTagEnd:

        // If we have node text then add this to the element
        if (text != nullptr) {
          size_t length = StripRight(text, token.pStr - text);
          TCHAR *text2 = FromXMLString(text, length);
          if (text2 == nullptr) {
            pXML->error = eXMLErrorUnexpectedToken;
            return false;
          }

          node.AddText(text2);
          free(text2);
          text = nullptr;
        }

        // Find the name of the end tag
        token = GetNextToken(pXML);

        // The end tag should be text
        if (token.type != eTokenText) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }

        // After the end tag we should find a closing tag
        if (GetNextToken(pXML).type != eTokenCloseTag) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }

        // We need to return to the previous caller.  If the name
        // of the tag cannot be found we need to keep returning to
        // caller until we find a match
        if (!CompareTagName(node.GetName(), token.pStr)) {
          pXML->lpEndTag = token.pStr;
          pXML->cbEndTag = token.length;
        }

        // Return to the caller
        return true;

        // Errors...
      case eTokenCloseTag: /* '>'         */
      case eTokenShortHandClose: /* '/>'        */
        pXML->error = eXMLErrorUnexpectedToken;
        return false;
      default:
        break;
      }
      break;

      // If we are inside a tag definition we need to search for attributes
    case eInsideTag:
      // Check what part of the attribute (name, equals, value) we
      // are looking for.
      switch (attrib) {
        // If we are looking for a new attribute
      case eAttribName:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text...
          // Eg.  'attribute'
        case eTokenText:
          // Cache the token then indicate that we are next to
          // look for the equals
          attribute_name.assign(token.pStr, token.length);
          attrib = eAttribEquals;
          break;

          // If we found a closing tag...
          // Eg.  '>'
        case eTokenCloseTag:
          // We are now outside the tag
          status = eOutsideTag;
          break;

          // If we found a short hand '/>' closing tag then we can
          // return to the caller
        case eTokenShortHandClose:
          return true;

          // Errors...
        case eTokenQuotedText: /* '"SomeText"'   */
        case eTokenTagStart: /* '<'            */
        case eTokenTagEnd: /* '</'           */
        case eTokenEquals: /* '='            */
        case eTokenDeclaration: /* '<?'           */
          pXML->error = eXMLErrorUnexpectedToken;
          return false;
        default:
          break;
        }
        break;

        // If we are looking for an equals
      case eAttribEquals:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text...
          // Eg.  'Attribute AnotherAttribute'
        case eTokenText:
          // Add the unvalued attribute to the list
          node.AddAttribute(std::move(attribute_name), _T(""), 0);
          // Cache the token then indicate.  We are next to
          // look for the equals attribute
          attribute_name.assign(token.pStr, token.length);
          break;

          // If we found a closing tag 'Attribute >' or a short hand
          // closing tag 'Attribute />'
        case eTokenShortHandClose:
        case eTokenCloseTag:
          assert(!attribute_name.empty());

          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && attribute_name.back() == _T('?')) {
            attribute_name.pop_back();
          }

          if (!attribute_name.empty())
            // Add the unvalued attribute to the list
            node.AddAttribute(std::move(attribute_name), _T(""), 0);

          // If this is the end of the tag then return to the caller
          if (token.type == eTokenShortHandClose)
            return true;

          // We are now outside the tag
          status = eOutsideTag;
          break;

          // If we found the equals token...
          // Eg.  'Attribute ='
        case eTokenEquals:
          // Indicate that we next need to search for the value
          // for the attribute
          attrib = eAttribValue;
          break;

          // Errors...
        case eTokenQuotedText: /* 'Attribute "InvalidAttr"'*/
        case eTokenTagStart: /* 'Attribute <'            */
        case eTokenTagEnd: /* 'Attribute </'           */
        case eTokenDeclaration: /* 'Attribute <?'           */
          pXML->error = eXMLErrorUnexpectedToken;
          return false;
        default:
          break;
        }
        break;

        // If we are looking for an attribute value
      case eAttribValue:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text or quoted text...
          // Eg.  'Attribute = "Value"' or 'Attribute = Value' or
          // 'Attribute = 'Value''.
        case eTokenText:
        case eTokenQuotedText:
          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && (token.pStr[token.length - 1]) == _T('?')) {
            token.length--;
          }

          // Add the valued attribute to the list
          if (token.type == eTokenQuotedText) {
            token.pStr++;
            token.length -= 2;
          }

          assert(!attribute_name.empty());

          {
            TCHAR *value = FromXMLString(token.pStr, token.length);
            if (value == nullptr) {
              pXML->error = eXMLErrorUnexpectedToken;
              return false;
            }

            node.AddAttribute(std::move(attribute_name),
                              value, _tcslen(value));
            free(value);
          }

          // Indicate we are searching for a new attribute
          attrib = eAttribName;
          break;

          // Errors...
        case eTokenTagStart: /* 'Attr = <'          */
        case eTokenTagEnd: /* 'Attr = </'         */
        case eTokenCloseTag: /* 'Attr = >'          */
        case eTokenShortHandClose: /* "Attr = />"         */
        case eTokenEquals: /* 'Attr = ='          */
        case eTokenDeclaration: /* 'Attr = <?'         */
          pXML->error = eXMLErrorUnexpectedToken;
          return false;
        default:
          break;
        }
      }
    }
  }
}

/**
 * Count the number of lines and columns in an XML string.
 */
static void
CountLinesAndColumns(const TCHAR *lpXML, size_t nUpto, XML::Results *pResults)
{
  assert(lpXML);
  assert(pResults);

  pResults->line = 1;
  pResults->column = 1;
  for (size_t n = 0; n < nUpto; n++) {
    TCHAR ch = lpXML[n];
    assert(ch);
    if (ch == _T('\n')) {
      pResults->line++;
      pResults->column = 1;
    } else
      pResults->column++;
  }
}

/**
 * Parses the given XML String and returns the main XMLNode
 * @param xml_string XML String
 * @param tag (?)
 * @param pResults XMLResult object to write in on error or success
 * @return The main XMLNode or empty XMLNode on error
 */
XMLNode *
XML::ParseString(const TCHAR *xml_string, Results *pResults)
{
  // If String is empty
  if (xml_string == nullptr) {
    // If XML::Results object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->line = 0;
      pResults->column = 0;
    }

    // -> Return empty XMLNode
    return nullptr;
  }

  Error error;
  XMLNode xnode = XMLNode::Null();
  Parser xml = { nullptr, 0, eXMLErrorNone, nullptr, 0, true, };

  xml.lpXML = xml_string;

  // Fill the XMLNode xnode with the parsed data of xml
  // note: xnode is now the document node, not the main XMLNode
  ParseXMLElement(xnode, &xml);
  error = xml.error;

  // If the document node does not have childnodes
  XMLNode *child = xnode.GetFirstChild();
  if (child == nullptr) {
    // If XML::Results object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->line = 0;
      pResults->column = 0;
    }

    // -> Return empty XMLNode
    return nullptr;
  } else {
    // Set the document's first childnode as new main node
    xnode = std::move(*child);
  }

  // If the new main node is the xml declaration
  // -> try to take the first childnode again
  if (xnode.IsDeclaration()) {
    // If the declaration does not have childnodes
    child = xnode.GetFirstChild();
    if (child == nullptr) {
      // If XML::Results object exists
      if (pResults) {
        // -> Save the error type
        pResults->error = eXMLErrorNoElements;
        pResults->line = 0;
        pResults->column = 0;
      }

      // -> Return empty XMLNode
      return nullptr;
    } else {
      // Set the declaration's first childnode as new main node
      xnode = std::move(*child);
    }
  }

  // If an XML::Results object exists
  // -> save the result (error/success)
  if (pResults) {
    pResults->error = error;

    // If we have an error
    if (error != eXMLErrorNone) {
      // Find which line and column it starts on and
      // save it in the XML::Results object
      CountLinesAndColumns(xml.lpXML, xml.nIndex, pResults);
    }
  }

  // If error occurred -> set node to empty
  if (error != eXMLErrorNone)
    return nullptr;

  // Return the node (empty, main or child of main that equals tag)
  return new XMLNode(std::move(xnode));
}

static bool
ReadTextFile(Path path, tstring &buffer)
try {
  /* auto-detect the character encoding, to be able to parse XCSoar
     6.0 task files */
  FileLineReader reader(path, Charset::AUTO);

  long size = reader.GetSize();
  if (size > 65536)
    return false;
  else if (size < 0)
    size = 4096;

  buffer.reserve(size);

  const TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (buffer.length() > 65536)
      /* too long */
      return false;

    buffer.append(line);
    buffer.append(_T("\n"));
  }

  return true;
} catch (const std::runtime_error &) {
  return false;
}

/**
* Opens the file given by the filepath and returns the main node.
* (Includes error handling)
 * @param filename Filepath to the XML file to parse
 * @param tag (?)
 * @param pResults Pointer to the XML::Results object to fill on error or success
 * @return The main XMLNode or an empty node on error
 */
XMLNode *
XML::ParseFile(Path filename, Results *pResults)
{
  // Open the file for reading
  tstring buffer;

  // If file can't be read
  if (!ReadTextFile(filename, buffer)) {
    // If XML::Results object exists
    if (pResults) {
      // -> Save the error type into it
      pResults->error = eXMLErrorFileNotFound;
      pResults->line = 0;
      pResults->column = 0;
    }

    // -> Return empty XMLNode
    return nullptr;
  }

  // Parse the string and get the main XMLNode
  return ParseString(buffer.c_str(), pResults);
}
