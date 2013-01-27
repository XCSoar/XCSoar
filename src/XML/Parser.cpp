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
#include "Util/StringUtil.hpp"
#include "Util/NumberParser.hpp"
#include "IO/FileLineReader.hpp"

#include <assert.h>
#include <stdio.h>

namespace XML {
  bool global_error = false;

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
  GetNextToken(Parser *pXML, size_t &token_length_r, TokenTypeTag &pType);

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
  assert(ss != NULL);

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
          XML::global_error = true;
          return NULL;
        }

        // XXX convert to UTF-8 if !_UNICODE
        TCHAR ch = (TCHAR)i;
        if (ch == 0)
          ch = ' ';

        *d++ = ch;
        ss = endptr + 1;
      } else {
        free(result);
        XML::global_error = true;
        return NULL;
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
  if (d != NULL)
    result = d;

  return result;
}

static bool
CompareTagName(const TCHAR *cclose, const TCHAR *copen)
{
  if (!cclose)
    return false;
  size_t l = _tcslen(cclose);
  if (!StringIsEqualIgnoreCase(cclose, copen, l))
    return false;

  const TCHAR c = copen[l];
  if ((c == _T('\n')) ||
      (c == _T(' ')) ||
      (c == _T('\t')) ||
      (c == _T('\r')) ||
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
    switch (ch) {
    // Ignore white space
    case _T('\n'):
    case _T(' '):
    case _T('\t'):
    case _T('\r'):
      continue;
    default:
      return ch;
    }
  }
  return 0;
}

/**
 * Find the next token in a string.
 *
 * @param token_length_r contains the number of characters that have been read
 */
static XML::NextToken
XML::GetNextToken(Parser *pXML, size_t &token_length_r, TokenTypeTag &pType)
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
    token_length_r = 0;
    pType = eTokenError;
    result.pStr = NULL;
    return result;
  }

  // Cache the current string pointer
  lpXML = pXML->lpXML;
  result.pStr = &lpXML[pXML->nIndex - 1];
  temp_ch = 0;

  switch (ch) {
    // Check for quotes
  case _T('\''):
  case _T('\"'):
    // Type of token
    pType = eTokenQuotedText;
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
      pXML->nIndex = n - 1;
      ch = GetNextChar(pXML);
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
    pType = eTokenEquals;
    break;

    // Close tag
  case _T('>'):
    size = 1;
    pType = eTokenCloseTag;
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
      pType = eTokenTagEnd;
      size = 2;
    }

    // If we have an XML declaration tag
    else if (temp_ch == _T('?')) {

      // Set the type and ensure we point at the next character
      GetNextChar(pXML);
      pType = eTokenDeclaration;
      size = 2;
    }

    // Otherwise we must have a start tag
    else {
      pType = eTokenTagStart;
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
      pType = eTokenShortHandClose;
      size = 2;
      break;
    }

    // If we haven't found a short hand closing tag then drop into the
    // text process

    // Other characters
  default:
    is_text = true;
  }

  // If this is a TEXT node
  if (is_text) {
    // Indicate we are dealing with text
    pType = eTokenText;
    size = 1;
    bool nExit = false;

    while (!nExit && ((ch = GetNextChar(pXML)) != 0)) {
      switch (ch) {
        // Break when we find white space
      case _T('\n'):
      case _T(' '):
      case _T('\t'):
      case _T('\r'):
        nExit = true;
      break;

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
  token_length_r = size;

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
 * Trim the end of the text to remove white space characters.
 */
gcc_pure
static size_t
FindEndOfText(const TCHAR *token, size_t length)
{
  assert(token != NULL);

  --length;
  while (1) {
    if (IsWhitespaceOrNull(token[length]))
      return length + 1;

    --length;
  }
}

/**
 * Recursively parse an XML element.
 */
static bool
XML::ParseXMLElement(XMLNode &node, Parser *pXML)
{
  const TCHAR *temp = NULL;
  size_t temp_length;
  bool is_declaration;
  const TCHAR *text = NULL;
  XMLNode *pNew;
  enum Status status; // inside or outside a tag
  enum Attrib attrib = eAttribName;

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
    size_t token_length;
    enum TokenTypeTag type;
    NextToken token = GetNextToken(pXML, token_length, type);
    if (gcc_unlikely(type == eTokenError))
      return false;

    // Check the current status
    switch (status) {
      // If we are outside of a tag definition
    case eOutsideTag:

      // Check what type of token we obtained
      switch (type) {
        // If we have found text or quoted text
      case eTokenText:
      case eTokenQuotedText:
      case eTokenEquals:
        if (text == NULL)
          text = token.pStr;

        break;

        // If we found a start tag '<' and declarations '<?'
      case eTokenTagStart:
      case eTokenDeclaration:
        // Cache whether this new element is a declaration or not
        is_declaration = type == eTokenDeclaration;

        // If we have node text then add this to the element
        if (text != NULL) {
          temp_length = FindEndOfText(text, token.pStr - text);
          node.AddText(text, temp_length);
          text = NULL;
        }

        // Find the name of the tag
        token = GetNextToken(pXML, token_length, type);

        // Return an error if we couldn't obtain the next token or
        // it wasnt text
        if (type != eTokenText) {
          pXML->error = eXMLErrorMissingTagName;
          return false;
        }

        // If the name of the new element differs from the name of
        // the current element we need to add the new element to
        // the current one and recurse
        pNew = &node.AddChild(token.pStr, token_length,
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
              if (node.GetName() == NULL) {
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
        if (text != NULL) {
          temp_length = FindEndOfText(text, token.pStr - text);
          TCHAR *text2 = FromXMLString(text, temp_length);
          if (text2 == NULL) {
            pXML->error = eXMLErrorUnexpectedToken;
            return false;
          }

          node.AddText(text2);
          free(text2);
          text = NULL;
        }

        // Find the name of the end tag
        token = GetNextToken(pXML, temp_length, type);

        // The end tag should be text
        if (type != eTokenText) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }
        temp = token.pStr;

        // After the end tag we should find a closing tag
        token = GetNextToken(pXML, token_length, type);
        if (type != eTokenCloseTag) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }

        // We need to return to the previous caller.  If the name
        // of the tag cannot be found we need to keep returning to
        // caller until we find a match
        if (!CompareTagName(node.GetName(), temp)) {
          pXML->lpEndTag = temp;
          pXML->cbEndTag = temp_length;
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
        switch (type) {
          // If the current type is text...
          // Eg.  'attribute'
        case eTokenText:
          // Cache the token then indicate that we are next to
          // look for the equals
          temp = token.pStr;
          temp_length = token_length;
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
        switch (type) {
          // If the current type is text...
          // Eg.  'Attribute AnotherAttribute'
        case eTokenText:
          // Add the unvalued attribute to the list
          node.AddAttribute(temp, temp_length, _T(""), 0);
          // Cache the token then indicate.  We are next to
          // look for the equals attribute
          temp = token.pStr;
          temp_length = token_length;
          break;

          // If we found a closing tag 'Attribute >' or a short hand
          // closing tag 'Attribute />'
        case eTokenShortHandClose:
        case eTokenCloseTag:
          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && (temp[temp_length - 1]) == _T('?'))
            temp_length--;

          if (temp_length)
            // Add the unvalued attribute to the list
            node.AddAttribute(temp, temp_length, _T(""), 0);

          // If this is the end of the tag then return to the caller
          if (type == eTokenShortHandClose)
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
        switch (type) {
          // If the current type is text or quoted text...
          // Eg.  'Attribute = "Value"' or 'Attribute = Value' or
          // 'Attribute = 'Value''.
        case eTokenText:
        case eTokenQuotedText:
          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && (token.pStr[token_length - 1]) == _T('?')) {
            token_length--;
          }

          if (temp_length) {
            // Add the valued attribute to the list
            if (type == eTokenQuotedText) {
              token.pStr++;
              token_length -= 2;
            }

            TCHAR *value = FromXMLString(token.pStr, token_length);
            node.AddAttribute(temp, temp_length,
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
  if (xml_string == NULL) {
    // If XML::Results object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->line = 0;
      pResults->column = 0;
    }

    // -> Return empty XMLNode
    return NULL;
  }

  Error error;
  XMLNode xnode = XMLNode::Null();
  Parser xml = { NULL, 0, eXMLErrorNone, NULL, 0, true, };

  xml.lpXML = xml_string;

  // Fill the XMLNode xnode with the parsed data of xml
  // note: xnode is now the document node, not the main XMLNode
  ParseXMLElement(xnode, &xml);
  error = xml.error;

  // If the document node does not have childnodes
  XMLNode *child = xnode.GetFirstChild();
  if (child == NULL) {
    // If XML::Results object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->line = 0;
      pResults->column = 0;
    }

    // -> Return empty XMLNode
    return NULL;
  } else {
    // Set the document's first childnode as new main node
    xnode = std::move(*child);
  }

  // If the new main node is the xml declaration
  // -> try to take the first childnode again
  if (xnode.IsDeclaration()) {
    // If the declaration does not have childnodes
    child = xnode.GetFirstChild();
    if (child == NULL) {
      // If XML::Results object exists
      if (pResults) {
        // -> Save the error type
        pResults->error = eXMLErrorNoElements;
        pResults->line = 0;
        pResults->column = 0;
      }

      // -> Return empty XMLNode
      return NULL;
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
    return NULL;

  // Return the node (empty, main or child of main that equals tag)
  return new XMLNode(std::move(xnode));
}

static bool
ReadTextFile(const TCHAR *path, tstring &buffer)
{
  /* auto-detect the character encoding, to be able to parse XCSoar
     6.0 task files */
  FileLineReader reader(path, ConvertLineReader::AUTO);
  if (reader.error())
    return false;

  long size = reader.GetSize();
  if (size > 65536)
    return false;
  else if (size < 0)
    size = 4096;

  buffer.reserve(size);

  const TCHAR *line;
  while ((line = reader.ReadLine()) != NULL) {
    if (buffer.length() > 65536)
      /* too long */
      return false;

    buffer.append(line);
    buffer.append(_T("\n"));
  }

  return true;
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
XML::ParseFile(const TCHAR *filename, Results *pResults)
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
    return NULL;
  }

  // Parse the string and get the main XMLNode
  return ParseString(buffer.c_str(), pResults);
}

/**
 * Opens the file given by the filepath and returns the main node.
 * (Includes error handling)
 * @param path Filepath to the XML file to parse
 * @param tag (?)
 * @return The main XMLNode
 */
XMLNode *
XML::OpenFileHelper(const TCHAR *path)
{
  Results pResults;
  global_error = false;

  // Parse the file and get the main XMLNode
  XMLNode *xnode = ParseFile(path, &pResults);

  // If error appeared
  if (pResults.error != eXMLErrorNone) {

    // In debug mode -> Log error to stdout
#ifdef DUMP_XML_ERRORS
    printf("XML Parsing error inside file '%s'.\n"
#ifdef _UNICODE
           "Error: %S\n"
#else
           "Error: %s\n"
#endif
           "At line %u, column %u.\n", path,
           GetErrorMessage(pResults.error),
           pResults.line, pResults.column);
#endif

    // Remember Error
    global_error = true;
  }

  // Return the parsed node or empty node on error
  return xnode;
}
