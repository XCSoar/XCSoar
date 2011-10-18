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

#if defined(WIN32)
#include <windows.h> // to have IsTextUnicode, MultiByteToWideChar, WideCharToMultiByte
                     // to handle unicode files
#endif

#include "xmlParser.hpp"
#include "Compatibility/string.h"
#include "IO/TextWriter.hpp"
#include "IO/FileLineReader.hpp"
#include "Util/StringUtil.hpp"
#include "Util/tstring.hpp"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

bool XMLNode::GlobalError = false;

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

#define INDENTCHAR '\t'

/** Main structure used for parsing XML. */
struct XML {
    const TCHAR *lpXML;
    unsigned nIndex;
    enum XMLError error;
    const TCHAR *lpEndTag;
    size_t cbEndTag;
    const TCHAR *lpNewElement;
    size_t cbNewElement;
    bool nFirst;
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

static void
write_xml_string(TextWriter &writer, const TCHAR *source)
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
fromXMLString(const TCHAR *ss, size_t lo)
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
      if (_tcsnicmp(ss, _T("lt;" ), 3) == 0) {
        *(d++) = _T('<' );
        ss += 3;
      } else if (_tcsnicmp(ss, _T("gt;" ), 3) == 0) {
        *(d++) = _T('>' );
        ss += 3;
      } else if (_tcsnicmp(ss, _T("amp;" ), 4) == 0) {
        *(d++) = _T('&' );
        ss += 4;
      } else if (_tcsnicmp(ss, _T("apos;"), 5) == 0) {
        *(d++) = _T('\'');
        ss += 5;
      } else if (_tcsnicmp(ss, _T("quot;"), 5) == 0) {
        *(d++) = _T('"' );
        ss += 5;
      } else if (*ss == '#') {
        /* number entity */

        ++ss;

        TCHAR *endptr;
        unsigned long i = _tcstoul(ss, &endptr, 10);
        if (endptr == ss || endptr >= end || *endptr != ';') {
          free(result);
          XMLNode::GlobalError = true;
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
        XMLNode::GlobalError = true;
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

/**
 * !!!! WARNING strange convention&:
 *
 * @return 0 if equals, 1 if different
 */
static char
myTagCompare(const TCHAR *cclose, const TCHAR *copen)
{
  if (!cclose)
    return 1;
  size_t l = _tcslen(cclose);
  if (_tcsnicmp(cclose, copen, l) != 0)
    return 1;

  const TCHAR c = copen[l];
  if ((c == _T('\n')) ||
      (c == _T(' ')) ||
      (c == _T('\t')) ||
      (c == _T('\r')) ||
      (c == _T('/')) ||
      (c == _T('<')) ||
      (c == _T('>')) ||
      (c == _T('=')))
    return 0;

  return 1;
}

void
XMLNode::removeOrderElement(XMLNodeData *d, XMLElementType t, unsigned index)
{
  unsigned j = (unsigned)((index << 2) + t), i = 0, n = nElement(d) + 1, *o = d->pOrder;
  while ((o[i] != j) && (i < n))
    i++;
  n--;
  memmove(o + i, o + i + 1, (n - i) * sizeof(unsigned));
  for (; i < n; i++)
    if ((o[i] & 3) == (unsigned)t)
      o[i] -= 4;
  // We should normally do:
  // d->pOrder=(int)realloc(d->pOrder,n*sizeof(int));
  // but we skip reallocation because it's too time consuming.
  // Anyway, at the end, it will be free'd completely at once.
}

/**
 * Obtain the next character from the string.
 */
static inline TCHAR
getNextChar(XML *pXML)
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
FindNonWhiteSpace(XML *pXML)
{
  assert(pXML);

    // Iterate through characters in the string until we find a NULL or a
  // non-white space character
  TCHAR ch;
  while ((ch = getNextChar(pXML)) != 0) {
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
 * @param pcbToken contains the number of characters that have been read
 */
static NextToken
GetNextToken(XML *pXML, size_t *pcbToken, enum TokenTypeTag *pType)
{
  NextToken result;
  const TCHAR *lpXML;
  TCHAR ch;
  TCHAR chTemp;
  size_t nSize;
  bool nFoundMatch;
  unsigned n;
  bool nIsText = false;

  // Find next non-white space character
  ch = FindNonWhiteSpace(pXML);
  if (gcc_unlikely(ch == 0)) {
    // If we failed to obtain a valid character
    *pcbToken = 0;
    *pType = eTokenError;
    result.pStr = NULL;
    return result;
  }

  // Cache the current string pointer
  lpXML = pXML->lpXML;
  result.pStr = &lpXML[pXML->nIndex - 1];
  chTemp = 0;

  switch (ch) {
    // Check for quotes
  case _T('\''):
  case _T('\"'):
    // Type of token
    *pType = eTokenQuotedText;
    chTemp = ch;
    n = pXML->nIndex;

    // Set the size
    nSize = 1;
    nFoundMatch = false;

    // Search through the string to find a matching quote
    while (((ch = getNextChar(pXML))) != 0) {
      nSize++;
      if (ch == chTemp) {
        nFoundMatch = true;
        break;
      }
      if (ch == _T('<'))
        break;
    }

    // If we failed to find a matching quote
    if (!nFoundMatch) {
      pXML->nIndex = n - 1;
      ch = getNextChar(pXML);
      nIsText = true;
      break;
    }

    //  4.02.2002
    if (FindNonWhiteSpace(pXML)) {
      pXML->nIndex--;
    }

    break;

    // Equals (used with attribute values)
  case _T('='):
    nSize = 1;
    *pType = eTokenEquals;
    break;

    // Close tag
  case _T('>'):
    nSize = 1;
    *pType = eTokenCloseTag;
    break;

    // Check for tag start and tag end
  case _T('<'):

    // Peek at the next character to see if we have an end tag '</',
    // or an xml declaration '<?'
    chTemp = pXML->lpXML[pXML->nIndex];

    // If we have a tag end...
    if (chTemp == _T('/')) {
      // Set the type and ensure we point at the next character
      getNextChar(pXML);
      *pType = eTokenTagEnd;
      nSize = 2;
    }

    // If we have an XML declaration tag
    else if (chTemp == _T('?')) {

      // Set the type and ensure we point at the next character
      getNextChar(pXML);
      *pType = eTokenDeclaration;
      nSize = 2;
    }

    // Otherwise we must have a start tag
    else {
      *pType = eTokenTagStart;
      nSize = 1;
    }
    break;

    // Check to see if we have a short hand type end tag ('/>').
  case _T('/'):

    // Peek at the next character to see if we have a short end tag '/>'
    chTemp = pXML->lpXML[pXML->nIndex];

    // If we have a short hand end tag...
    if (chTemp == _T('>')) {
      // Set the type and ensure we point at the next character
      getNextChar(pXML);
      *pType = eTokenShortHandClose;
      nSize = 2;
      break;
    }

    // If we haven't found a short hand closing tag then drop into the
    // text process

    // Other characters
  default:
    nIsText = true;
  }

  // If this is a TEXT node
  if (nIsText) {
    // Indicate we are dealing with text
    *pType = eTokenText;
    nSize = 1;
    bool nExit = false;

    while (!nExit && ((ch = getNextChar(pXML)) != 0)) {
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
        chTemp = pXML->lpXML[pXML->nIndex];

        // If we found a short hand end tag then we need to exit the loop
        if (chTemp == _T('>')) {
          pXML->nIndex--; //  03.02.2002
          nExit = true;
        } else {
          nSize++;
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
        nSize++;
      }
    }
  }
  *pcbToken = nSize;

  return result;
}

const TCHAR *
XMLNode::getError(XMLError error)
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


XMLNode
XMLNode::createRoot(const TCHAR *lpszName)
{
  return XMLNode(NULL, lpszName, false);
}

XMLNode::XMLNode(XMLNode *pParent, const TCHAR *lpszName, bool isDeclaration)
{
  d = (XMLNodeData*)malloc(sizeof(XMLNodeData));
  assert(d);
  d->ref_count = 1;

  d->lpszName = lpszName;

  d->nChild = 0;
  d->nText = 0;
  d->nAttribute = 0;

  d->isDeclaration = isDeclaration;

  d->pParent = pParent;
  d->pChild = NULL;
  d->pText = NULL;
  d->pAttribute = NULL;
  d->pOrder = NULL;
}

static const size_t memoryIncrease = 50;

static void *
myRealloc(void *p, size_t newsize, size_t memInc, size_t sizeofElem)
{
  size_t blocks = newsize / memInc + 1;
  if (p == NULL) {
    void* v = malloc(blocks * memInc * sizeofElem);
    assert(v);
    return v;
  }
  if ((newsize % memInc) == 0) {
    p = realloc(p, blocks * memInc * sizeofElem);
    assert(p);
  }
  return p;
}

void
XMLNode::addToOrder(unsigned index, unsigned type)
{
  unsigned n = nElement();
  d->pOrder = (unsigned *)myRealloc(d->pOrder, n + 1, memoryIncrease * 3,
                                    sizeof(unsigned));
  assert(d->pOrder);
  d->pOrder[n] = (index << 2) + type;
}

XMLNode
XMLNode::AddChild(const TCHAR *lpszName, bool isDeclaration)
{
  assert(lpszName != NULL);

  unsigned nc = d->nChild;
  d->pChild = (XMLNode*)myRealloc(d->pChild, (nc + 1), memoryIncrease,
                                  sizeof(XMLNode));
  assert(d->pChild);
  d->pChild[nc].d = NULL;
  d->pChild[nc] = XMLNode(this, lpszName, isDeclaration);
  addToOrder(nc, eNodeChild);
  d->nChild++;
  return d->pChild[nc];
}

void
XMLNode::AddAttribute(const TCHAR *lpszName, const TCHAR *lpszValuev)
{
  assert(lpszName != NULL);

  unsigned na = d->nAttribute;
  d->pAttribute = (XMLAttribute*)myRealloc(d->pAttribute, (na + 1),
                                           memoryIncrease, sizeof(XMLAttribute));
  XMLAttribute *pAttr = d->pAttribute + na;
  pAttr->lpszName = lpszName;
  pAttr->lpszValue = lpszValuev;
  addToOrder(na, eNodeAttribute);
  d->nAttribute++;
}

const TCHAR *
XMLNode::AddText(const TCHAR *lpszValue)
{
  assert(lpszValue != NULL);

  unsigned nt = d->nText;
  d->pText = (const TCHAR **)myRealloc(d->pText, (nt + 1), memoryIncrease,
                                       sizeof(const TCHAR *));
  d->pText[nt] = lpszValue;
  addToOrder(nt, eNodeText);
  d->nText++;
  return d->pText[nt];
}

/**
 * Trim the end of the text to remove white space characters.
 */
static void
FindEndOfText(const TCHAR *lpszToken, size_t *pcbText)
{
  assert(lpszToken);
  assert(pcbText);

  size_t cbText = (*pcbText) - 1;
  while (1) {
    TCHAR ch = lpszToken[cbText];
    switch (ch) {
    case _T('\r'):
    case _T('\n'):
    case _T('\t'):
    case _T(' '):
      cbText--;
      break;
    default:
      *pcbText = cbText + 1;
      return;
    }
  }
}


/**
 * Duplicate (copy in a new allocated buffer) the source string.
 */
static TCHAR *
stringDup(const TCHAR *lpszData, size_t cbData)
{
  assert(lpszData != NULL);

  TCHAR *lpszNew = (TCHAR *)malloc((cbData + 1) * sizeof(TCHAR));
  assert(lpszNew);
  if (lpszNew) {
    memcpy(lpszNew, lpszData, (cbData) * sizeof(TCHAR));
    lpszNew[cbData] = (TCHAR)NULL;
  }
  return lpszNew;
}

/**
 * Recursively parse an XML element.
 */
bool
XMLNode::ParseXMLElement(XML *pXML)
{
  const TCHAR *lpszTemp = NULL;
  size_t cbTemp;
  unsigned nDeclaration;
  const TCHAR *lpszText = NULL;
  XMLNode pNew;
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
    size_t cbToken;
    enum TokenTypeTag type;
    NextToken token = GetNextToken(pXML, &cbToken, &type);
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
        if (!lpszText)
          lpszText = token.pStr;

        break;

        // If we found a start tag '<' and declarations '<?'
      case eTokenTagStart:
      case eTokenDeclaration:
        // Cache whether this new element is a declaration or not
        nDeclaration = type == eTokenDeclaration;

        // If we have node text then add this to the element
        if (lpszText) {
          cbTemp = token.pStr - lpszText;
          FindEndOfText(lpszText, &cbTemp);
          AddText(stringDup(lpszText, cbTemp));
          lpszText = NULL;
        }

        // Find the name of the tag
        token = GetNextToken(pXML, &cbToken, &type);

        // Return an error if we couldn't obtain the next token or
        // it wasnt text
        if (type != eTokenText) {
          pXML->error = eXMLErrorMissingTagName;
          return false;
        }

        // If we found a new element which is the same as this
        // element then we need to pass this back to the caller..

#ifdef APPROXIMATE_PARSING
        if (d->lpszName && myTagCompare(d->lpszName, token.pStr) == 0) {
          // Indicate to the caller that it needs to create a
          // new element.
          pXML->lpNewElement = token.pStr;
          pXML->cbNewElement = cbToken;
          return true;
        }
#endif

        // If the name of the new element differs from the name of
        // the current element we need to add the new element to
        // the current one and recurse
        pNew = AddChild(stringDup(token.pStr, cbToken), nDeclaration);

        while (true) {
          // Callself to process the new node.  If we return
          // FALSE this means we dont have any more
          // processing to do...

          if (!pNew.ParseXMLElement(pXML)) {
            d->pOrder = (unsigned *)myRealloc(d->pOrder, nElement(),
                                              memoryIncrease * 3,
                                              sizeof(unsigned));
            d->pChild = (XMLNode*)myRealloc(d->pChild, d->nChild,
                                            memoryIncrease, sizeof(XMLNode));
            if (d->nAttribute > 0)
              d->pAttribute = (XMLAttribute*)myRealloc(d->pAttribute,
                                                       d->nAttribute,
                                                       memoryIncrease,
                                                       sizeof(XMLAttribute));
            if (d->nText > 0)
              d->pText = (const TCHAR **)myRealloc(d->pText, d->nText,
                                                   memoryIncrease,
                                                   sizeof(const TCHAR *));
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
              if (!d->lpszName) {
                pXML->error = eXMLErrorUnmatchedEndTag;
                return false;
              }

              // If the end tag matches the name of this
              // element then we only need to unwind
              // once more...

              if (myTagCompare(d->lpszName, pXML->lpEndTag) == 0) {
                pXML->cbEndTag = 0;
              }

              return true;
            } else if (pXML->cbNewElement) {
              // If the call indicated a new element is to
              // be created on THIS element.

              // If the name of this element matches the
              // name of the element we need to create
              // then we need to return to the caller
              // and let it process the element.

              if (myTagCompare(d->lpszName, pXML->lpNewElement) == 0)
                return true;

              // Add the new element and recurse
              pNew = AddChild(stringDup(pXML->lpNewElement,
                                        pXML->cbNewElement), false);
              pXML->cbNewElement = 0;
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
        if (lpszText) {
          cbTemp = token.pStr - lpszText;
          FindEndOfText(lpszText, &cbTemp);
          TCHAR *text = fromXMLString(lpszText, cbTemp);
          if (text == NULL) {
            pXML->error = eXMLErrorUnexpectedToken;
            return false;
          }

          AddText(text);
          lpszText = NULL;
        }

        // Find the name of the end tag
        token = GetNextToken(pXML, &cbTemp, &type);

        // The end tag should be text
        if (type != eTokenText) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }
        lpszTemp = token.pStr;

        // After the end tag we should find a closing tag
        token = GetNextToken(pXML, &cbToken, &type);
        if (type != eTokenCloseTag) {
          pXML->error = eXMLErrorMissingEndTagName;
          return false;
        }

        // We need to return to the previous caller.  If the name
        // of the tag cannot be found we need to keep returning to
        // caller until we find a match
        if (myTagCompare(d->lpszName, lpszTemp) != 0) {
          pXML->lpEndTag = lpszTemp;
          pXML->cbEndTag = cbTemp;
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
          lpszTemp = token.pStr;
          cbTemp = cbToken;
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
          AddAttribute(stringDup(lpszTemp, cbTemp), NULL);
          // Cache the token then indicate.  We are next to
          // look for the equals attribute
          lpszTemp = token.pStr;
          cbTemp = cbToken;
          break;

          // If we found a closing tag 'Attribute >' or a short hand
          // closing tag 'Attribute />'
        case eTokenShortHandClose:
        case eTokenCloseTag:
          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (d->isDeclaration && (lpszTemp[cbTemp - 1]) == _T('?'))
            cbTemp--;

          if (cbTemp)
            // Add the unvalued attribute to the list
            AddAttribute(stringDup(lpszTemp, cbTemp), NULL);

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
          if (d->isDeclaration && (token.pStr[cbToken - 1]) == _T('?')) {
            cbToken--;
          }

          if (cbTemp) {
            // Add the valued attribute to the list
            if (type == eTokenQuotedText) {
              token.pStr++;
              cbToken -= 2;
            }
            AddAttribute(stringDup(lpszTemp, cbTemp), fromXMLString(
                                                                    token.pStr, cbToken));
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
CountLinesAndColumns(const TCHAR *lpXML, size_t nUpto, XMLResults *pResults)
{
  assert(lpXML);
  assert(pResults);

  pResults->nLine = 1;
  pResults->nColumn = 1;
  for (size_t n = 0; n < nUpto; n++) {
    TCHAR ch = lpXML[n];
    assert(ch);
    if (ch == _T('\n')) {
      pResults->nLine++;
      pResults->nColumn = 1;
    } else
      pResults->nColumn++;
  }
}

/**
 * Parses the given XML String (lpszXML) and returns the main XMLNode
 * @param lpszXML XML String
 * @param tag (?)
 * @param pResults XMLResult object to write in on error or success
 * @return The main XMLNode or empty XMLNode on error
 */
XMLNode *
XMLNode::parseString(const TCHAR *lpszXML, XMLResults *pResults)
{
  // If String is empty
  if (!lpszXML) {
    // If XMLResults object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->nLine = 0;
      pResults->nColumn = 0;
    }

    // -> Return empty XMLNode
    return NULL;
  }

  enum XMLError error;
  XMLNode xnode(NULL, NULL, false);
  struct XML xml = { NULL, 0, eXMLErrorNone, NULL, 0, NULL, 0, true, };

  xml.lpXML = lpszXML;

  // Fill the XMLNode xnode with the parsed data of xml
  // note: xnode is now the document node, not the main XMLNode
  xnode.ParseXMLElement(&xml);
  error = xml.error;

  // If the document node does not have childnodes
  const XMLNode *child = xnode.getChildNode(0u);
  if (child == NULL) {
    // If XMLResults object exists
    if (pResults) {
      // -> Save the error type
      pResults->error = eXMLErrorNoElements;
      pResults->nLine = 0;
      pResults->nColumn = 0;
    }

    // -> Return empty XMLNode
    return NULL;
  } else {
    // Set the document's first childnode as new main node
    xnode = XMLNode(*child);
  }

  // If the new main node is the xml declaration
  // -> try to take the first childnode again
  if (xnode.isDeclaration()) {
    // If the declaration does not have childnodes
    child = xnode.getChildNode(0u);
    if (child == NULL) {
      // If XMLResults object exists
      if (pResults) {
        // -> Save the error type
        pResults->error = eXMLErrorNoElements;
        pResults->nLine = 0;
        pResults->nColumn = 0;
      }

      // -> Return empty XMLNode
      return NULL;
    } else {
      // Set the declaration's first childnode as new main node
      xnode = XMLNode(*child);
    }
  }

  // If an XMLResults object exists
  // -> save the result (error/success)
  if (pResults) {
    pResults->error = error;

    // If we have an error
    if (error != eXMLErrorNone) {
      // Find which line and column it starts on and
      // save it in the XMLResults object
      CountLinesAndColumns(xml.lpXML, xml.nIndex, pResults);
    }
  }

  // If error occurred -> set node to empty
  if (error != eXMLErrorNone)
    return NULL;

  // Return the node (empty, main or child of main that equals tag)
  return new XMLNode(xnode);
}

static bool
read_text_file(const char *path, tstring &buffer)
{
  /* auto-detect the character encoding, to be able to parse XCSoar
     6.0 task files */
  FileLineReader reader(path, ConvertLineReader::AUTO);
  if (reader.error())
    return false;

  long size = reader.size();
  if (size > 65536)
    return false;
  else if (size < 0)
    size = 4096;

  buffer.reserve(size);

  const TCHAR *line;
  while ((line = reader.read()) != NULL) {
    if (buffer.length() > 65536)
      /* too long */
      return false;

    buffer.append(line);
    buffer.append(_T("\n"));
  }

  return true;
}

/**
* Opens the file given by the filepath in lpszXML and returns the main node.
* (Includes error handling)
 * @param filename Filepath to the XML file to parse
 * @param tag (?)
 * @param pResults Pointer to the XMLResults object to fill on error or success
 * @return The main XMLNode or an empty node on error
 */
XMLNode *
XMLNode::parseFile(const char *filename, XMLResults *pResults)
{
  // Open the file for reading
  tstring buffer;

  // If file can't be read
  if (!read_text_file(filename, buffer)) {
    // If XMLResults object exists
    if (pResults) {
      // -> Save the error type into it
      pResults->error = eXMLErrorFileNotFound;
      pResults->nLine = 0;
      pResults->nColumn = 0;
    }

    // -> Return empty XMLNode
    return NULL;
  }

  // Parse the string and get the main XMLNode
  return parseString(buffer.c_str(), pResults);
}

/**
 * Opens the file given by the filepath in lpszXML and returns the main node.
 * (Includes error handling)
 * @param lpszXML Filepath to the XML file to parse
 * @param tag (?)
 * @return The main XMLNode
 */
XMLNode *
XMLNode::openFileHelper(const char *lpszXML)
{
  XMLResults pResults;
  XMLNode::GlobalError = false;

  // Parse the file and get the main XMLNode
  XMLNode *xnode = XMLNode::parseFile(lpszXML, &pResults);

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
           "At line %u, column %u.\n", lpszXML,
           XMLNode::getError(pResults.error), pResults.nLine, pResults.nColumn);
#endif

    // Remember Error
    XMLNode::GlobalError = true;
  }

  // Return the parsed node or empty node on error
  return xnode;
}

const void *
XMLNode::enumContent(const XMLNodeData *pEntry, unsigned i,
                     XMLElementType *nodeType)
{
  XMLElementType j = (XMLElementType)(pEntry->pOrder[i] & 3);
  *nodeType = j;
  i = (pEntry->pOrder[i]) >> 2;
  switch (j) {
  case eNodeChild:
    return pEntry->pChild[i].d;
  case eNodeAttribute:
    return pEntry->pAttribute + i;
  case eNodeText:
    return (void*)(pEntry->pText[i]);
  default:
    break;
  }
  return NULL;
}

unsigned
XMLNode::nElement(const XMLNodeData *pEntry)
{
  return pEntry->nChild + pEntry->nText + pEntry->nAttribute;
}

static inline void
charmemset(TCHAR *dest, TCHAR c, size_t l)
{
  while (l--)
    *(dest++) = c;
}

static void
write_indent(TextWriter &writer, unsigned n)
{
  while (n-- > 0)
    writer.write(INDENTCHAR);
}

void
XMLNode::serialiseR(const XMLNodeData *pEntry, TextWriter &writer, int nFormat)
{
  unsigned cb;
  unsigned nIndex;
  int nChildFormat = -1;
  bool bHasChildren = false;
  unsigned i;
  const XMLAttribute *pAttr;

  assert(pEntry);

  // If the element has no name then assume this is the head node.
  if (!string_is_empty(pEntry->lpszName)) {
    // "<elementname "
    cb = nFormat == -1 ? 0 : nFormat;

    write_indent(writer, cb);
    writer.write('<');
    if (pEntry->isDeclaration)
      writer.write('?');
    writer.write(pEntry->lpszName);

    // Enumerate attributes and add them to the string
    nIndex = pEntry->nAttribute;
    pAttr = pEntry->pAttribute;
    for (i = 0; i < nIndex; i++) {
      writer.write(' ');
      writer.write(pAttr->lpszName);
      writer.write('=');
      writer.write('"');
      if (pAttr->lpszValue != NULL)
        write_xml_string(writer, pAttr->lpszValue);
      writer.write('"');
      pAttr++;
    }

    bHasChildren = (pEntry->nAttribute != nElement(pEntry));
    if (pEntry->isDeclaration) {
      writer.write('?');
      writer.write('>');
      if (nFormat != -1)
        writer.newline();
    } else
    // If there are child nodes we need to terminate the start tag
    if (bHasChildren) {
      writer.write('>');
      if (nFormat != -1)
        writer.newline();
    }
  }

  // Calculate the child format for when we recurse.  This is used to
  // determine the number of spaces used for prefixes.
  if (nFormat != -1) {
    if (!string_is_empty(pEntry->lpszName))
      nChildFormat = nFormat + 1;
    else
      nChildFormat = nFormat;
  }

  // Enumerate through remaining children
  nIndex = nElement(pEntry);
  XMLElementType nodeType;
  const void *pChild;
  for (i = 0; i < nIndex; i++) {
    pChild = enumContent(pEntry, i, &nodeType);
    switch (nodeType) {
    // Text nodes
    case eNodeText:
      // "Text"
      if (!string_is_empty((const TCHAR *)pChild)) {
        if (nFormat != -1) {
          write_indent(writer, nFormat + 1);
          write_xml_string(writer, (const TCHAR *)pChild);
          writer.newline();
        } else {
          write_xml_string(writer, (const TCHAR *)pChild);
        }
      }
      break;

      // Element nodes
    case eNodeChild:

      // Recursively add child nodes
      serialiseR((const XMLNodeData*)pChild, writer, nChildFormat);
      break;

    default:
      break;
    }
  }

  if (!string_is_empty(pEntry->lpszName) && !pEntry->isDeclaration) {
    // If we have child entries we need to use long XML notation for
    // closing the element - "<elementname>blah blah blah</elementname>"
    if (bHasChildren) {
      // "</elementname>\0"
      if (nFormat != -1)
        write_indent(writer, nFormat);

      writer.write("</");
      writer.write(pEntry->lpszName);

      writer.write('>');
    } else {
      // If there are no children we can use shorthand XML notation -
      // "<elementname/>"
      // "/>\0"
      writer.write("/>");
    }

    if (nFormat != -1)
      writer.newline();
  }
}

void
XMLNode::serialise(TextWriter &writer, int nFormat) const
{
  nFormat = nFormat ? 0 : -1;
  serialiseR(d, writer, nFormat);
}

XMLNode::~XMLNode()
{
  destroyCurrentBuffer(d);
}

void
XMLNode::destroyCurrentBuffer(XMLNodeData *d)
{
  if (!d)
    return;

  (d->ref_count)--;

  if (d->ref_count == 0) {
    unsigned i = 0;

    if (d->pParent) {
      XMLNode *pa = d->pParent->d->pChild;
      while (((void*)(pa[i].d)) != ((void*)d))
        i++;
      d->pParent->d->nChild--;
      memmove(pa + i, pa + i + 1, (d->pParent->d->nChild - i) * sizeof(XMLNode));
      removeOrderElement(d->pParent->d, eNodeChild, i);
    }

    for (i = 0; i < d->nChild; i++) {
      d->pChild[i].d->pParent = NULL;
      destroyCurrentBuffer(d->pChild[i].d);
    }
    free(d->pChild);
    for (i = 0; i < d->nText; i++)
      free((void*)d->pText[i]);
    free(d->pText);
    for (i = 0; i < d->nAttribute; i++) {
      free((void*)d->pAttribute[i].lpszName);
      if (d->pAttribute[i].lpszValue)
        free((void*)d->pAttribute[i].lpszValue);
    }
    free(d->pAttribute);
    free(d->pOrder);
    free((void*)d->lpszName);
    free(d);
  }
}

XMLNode&
XMLNode::operator=(const XMLNode& A)
{
  if (this != &A) {
    destroyCurrentBuffer(d);
    d = A.d;
    if (d)
      (d->ref_count)++;
  }
  return *this;
}

XMLNode::XMLNode(const XMLNode &A)
{
  d = A.d;
  if (d)
    (d->ref_count)++;
}

const XMLNode *
XMLNode::getChildNode(const TCHAR *name, unsigned *j) const
{
  if (!d)
    return NULL;

  unsigned i = 0, n = d->nChild;
  if (j)
    i = *j;
  XMLNode *pc = d->pChild + i;
  for (; i < n; i++) {
    if (_tcsicmp(pc->d->lpszName, name) == 0) {
      if (j)
        *j = i + 1;
      return pc;
    }
    pc++;
  }

  return NULL;
}

const XMLNode *
XMLNode::getChildNode(const TCHAR *name, unsigned j) const
{
  if (!d)
    return NULL;

  unsigned i = 0;
  while (j-- > 0)
    getChildNode(name, &i);

  return getChildNode(name, &i);
}

const TCHAR *
XMLNode::getAttribute(const TCHAR *lpszAttrib, unsigned *j) const
{
  if (!d)
    return NULL;

  unsigned i = 0, n = d->nAttribute;
  if (j)
    i = *j;

  XMLAttribute *pAttr = d->pAttribute + i;
  for (; i < n; i++) {
    if (_tcsicmp(pAttr->lpszName, lpszAttrib) == 0) {
      if (j)
        *j = i + 1;
      return pAttr->lpszValue;
    }
    pAttr++;
  }

  return NULL;
}

const TCHAR *
XMLNode::getAttribute(const TCHAR *name, unsigned j) const
{
  if (!d)
    return NULL;

  unsigned i = 0;
  while (j-- > 0)
    getAttribute(name, &i);

  return getAttribute(name, &i);
}

const XMLNode *
XMLNode::getChildNode(unsigned i) const
{
  if (!d)
    return NULL;
  if (i >= d->nChild)
    return NULL;

  return &d->pChild[i];
}
