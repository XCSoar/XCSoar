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
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"
#include "util/StringStrip.hxx"
#include "util/NumberParser.hpp"
#include "io/FileLineReader.hpp"

#include <stdexcept>

#include <cassert>

namespace XML {

/** Main structure used for parsing XML. */
struct Parser {
  const TCHAR *lpXML;
  unsigned nIndex = 0;
  std::basic_string_view<TCHAR> end_tag{};
  bool nFirst = true;

  explicit constexpr Parser(const TCHAR *_xml) noexcept
    :lpXML(_xml) {}
};

/** Enumeration used to decipher what type a token is. */
enum class TokenType {
  TEXT,
  QUOTED_TEXT,
  TAG_START,         /* "<"            */
  TAG_END,           /* "</"           */
  CLOSE_TAG,         /* ">"            */
  EQUALS,           /* "="            */
  DECLARATION,      /* "<?"           */
  SHORT_HAND_CLOSE,   /* "/>"           */
  ERROR,
};

struct NextToken {
  std::basic_string_view<TCHAR> text;

  TokenType type;
};

/** Enumeration used when parsing attributes. */
enum class Attrib {
  NAME,
  EQUALS,
  VALUE
};

/**
 * Enumeration used when parsing elements to dictate whether we are
 * currently inside a tag.
 */
enum class Status {
  INSIDE_TAG,
  OUTSIDE_TAG
};

static NextToken
GetNextToken(Parser *pXML);

static void
ParseXMLElement(XMLNode &node, Parser *pXML);

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
FromXMLString(std::basic_string_view<TCHAR> src) noexcept
{
  const TCHAR *ss = src.data();
  const TCHAR *end = ss + src.size();

  /* allocate a buffer with the size of the input string; we know for
     sure that this is enough, because resolving entities can only
     shrink the string, but never grows */
  TCHAR *d = (TCHAR *)malloc((src.size() + 1) * sizeof(*d));
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
GetNextChar(Parser *pXML)
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
FindNonWhiteSpace(Parser *pXML)
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
static NextToken
GetNextToken(Parser *pXML)
{
  NextToken result;
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
    return {{}, TokenType::ERROR};
  }

  // Cache the current string pointer
  lpXML = pXML->lpXML;
  const TCHAR *const pStr = &lpXML[pXML->nIndex - 1];

  switch (ch) {
    // Check for quotes
  case _T('\''):
  case _T('\"'):
    // Type of token
    result.type = TokenType::QUOTED_TEXT;
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
    result.type = TokenType::EQUALS;
    break;

    // Close tag
  case _T('>'):
    size = 1;
    result.type = TokenType::CLOSE_TAG;
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
      result.type = TokenType::TAG_END;
      size = 2;
    }

    // If we have an XML declaration tag
    else if (temp_ch == _T('?')) {

      // Set the type and ensure we point at the next character
      GetNextChar(pXML);
      result.type = TokenType::DECLARATION;
      size = 2;
    }

    // Otherwise we must have a start tag
    else {
      result.type = TokenType::TAG_START;
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
      result.type = TokenType::SHORT_HAND_CLOSE;
      size = 2;
      break;
    }

    // If we haven't found a short hand closing tag then drop into the
    // text process

    [[fallthrough]];

    // Other characters
  default:
    is_text = true;
  }

  // If this is a TEXT node
  if (is_text) {
    // Indicate we are dealing with text
    result.type = TokenType::TEXT;
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
  result.text = {pStr, size};

  return result;
}

/**
 * Recursively parse an XML element.
 */
static void
ParseXMLElement(XMLNode &node, Parser *pXML)
{
  bool is_declaration;
  const TCHAR *text = nullptr;
  XMLNode *pNew;
  Status status; // inside or outside a tag
  Attrib attrib = Attrib::NAME;

  /* the name of the attribute that is currently being */
  tstring attribute_name;

  assert(pXML);

  // If this is the first call to the function
  if (pXML->nFirst) {
    // Assume we are outside of a tag definition
    pXML->nFirst = false;
    status = Status::OUTSIDE_TAG;
  } else {
    // If this is not the first call then we should only be called when inside a tag.
    status = Status::INSIDE_TAG;
  }

  // Iterate through the tokens in the document
  while (true) {
    // Obtain the next token
    NextToken token = GetNextToken(pXML);
    if (gcc_unlikely(token.type == TokenType::ERROR))
      return;

    // Check the current status
    switch (status) {
      // If we are outside of a tag definition
    case Status::OUTSIDE_TAG:

      // Check what type of token we obtained
      switch (token.type) {
        // If we have found text or quoted text
      case TokenType::TEXT:
      case TokenType::QUOTED_TEXT:
      case TokenType::EQUALS:
        if (text == nullptr)
          text = token.text.data();

        break;

        // If we found a start tag '<' and declarations '<?'
      case TokenType::TAG_START:
      case TokenType::DECLARATION:
        // Cache whether this new element is a declaration or not
        is_declaration = token.type == TokenType::DECLARATION;

        // If we have node text then add this to the element
        if (text != nullptr) {
          size_t length = StripRight(text, token.text.data() - text);
          node.AddText({text, length});
          text = nullptr;
        }

        // Find the name of the tag
        token = GetNextToken(pXML);

        // Return an error if we couldn't obtain the next token or
        // it wasnt text
        if (token.type != TokenType::TEXT)
          throw std::runtime_error("Missing start tag name");

        // If the name of the new element differs from the name of
        // the current element we need to add the new element to
        // the current one and recurse
        pNew = &node.AddChild(token.text, is_declaration);

        while (true) {
          // Callself to process the new node.  If we return
          // FALSE this means we dont have any more
          // processing to do...

          ParseXMLElement(*pNew, pXML);

          // If the call to recurse this function
          // evented in a end tag specified in XML then
          // we need to unwind the calls to this
          // function until we find the appropriate node
          // (the element name and end tag name must
          // match)
          if (!pXML->end_tag.empty()) {
            // If we are back at the root node then we
            // have an unmatched end tag
            if (node.GetName() == nullptr)
              throw std::runtime_error("Unmatched end tag");

            // If the end tag matches the name of this
            // element then we only need to unwind
            // once more...

            if (CompareTagName(node.GetName(), pXML->end_tag.data())) {
              pXML->end_tag = {};
            }

            return;
          } else {
            // If we didn't have a new element to create
            break;
          }
        }
        break;

        // If we found an end tag
      case TokenType::TAG_END:

        // If we have node text then add this to the element
        if (text != nullptr) {
          size_t length = StripRight(text, token.text.data() - text);
          TCHAR *text2 = FromXMLString({text, length});
          if (text2 == nullptr)
            throw std::runtime_error("Unexpected token found");

          node.AddText(text2);
          free(text2);
          text = nullptr;
        }

        // Find the name of the end tag
        token = GetNextToken(pXML);

        // The end tag should be text
        if (token.type != TokenType::TEXT)
          throw std::runtime_error("Missing end tag name");

        // After the end tag we should find a closing tag
        if (GetNextToken(pXML).type != TokenType::CLOSE_TAG)
          throw std::runtime_error("Missing end tag name");

        // We need to return to the previous caller.  If the name
        // of the tag cannot be found we need to keep returning to
        // caller until we find a match
        if (!CompareTagName(node.GetName(), token.text.data())) {
          pXML->end_tag = token.text;
        }

        // Return to the caller
        return;

        // Errors...
      case TokenType::CLOSE_TAG: /* '>'         */
      case TokenType::SHORT_HAND_CLOSE: /* '/>'        */
        throw std::runtime_error("Unexpected token found");
      default:
        break;
      }
      break;

      // If we are inside a tag definition we need to search for attributes
    case Status::INSIDE_TAG:
      // Check what part of the attribute (name, equals, value) we
      // are looking for.
      switch (attrib) {
        // If we are looking for a new attribute
      case Attrib::NAME:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text...
          // Eg.  'attribute'
        case TokenType::TEXT:
          // Cache the token then indicate that we are next to
          // look for the equals
          attribute_name = token.text;
          attrib = Attrib::EQUALS;
          break;

          // If we found a closing tag...
          // Eg.  '>'
        case TokenType::CLOSE_TAG:
          // We are now outside the tag
          status = Status::OUTSIDE_TAG;
          break;

          // If we found a short hand '/>' closing tag then we can
          // return to the caller
        case TokenType::SHORT_HAND_CLOSE:
          return;

          // Errors...
        case TokenType::QUOTED_TEXT: /* '"SomeText"'   */
        case TokenType::TAG_START: /* '<'            */
        case TokenType::TAG_END: /* '</'           */
        case TokenType::EQUALS: /* '='            */
        case TokenType::DECLARATION: /* '<?'           */
          throw std::runtime_error("Unexpected token found");
        default:
          break;
        }
        break;

        // If we are looking for an equals
      case Attrib::EQUALS:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text...
          // Eg.  'Attribute AnotherAttribute'
        case TokenType::TEXT:
          // Add the unvalued attribute to the list
          node.AddAttribute(std::move(attribute_name),
                            std::basic_string_view<TCHAR>{});
          // Cache the token then indicate.  We are next to
          // look for the equals attribute
          attribute_name = token.text;
          break;

          // If we found a closing tag 'Attribute >' or a short hand
          // closing tag 'Attribute />'
        case TokenType::SHORT_HAND_CLOSE:
        case TokenType::CLOSE_TAG:
          assert(!attribute_name.empty());

          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && attribute_name.back() == _T('?')) {
            attribute_name.pop_back();
          }

          if (!attribute_name.empty())
            // Add the unvalued attribute to the list
            node.AddAttribute(std::move(attribute_name),
                              std::basic_string_view<TCHAR>{});

          // If this is the end of the tag then return to the caller
          if (token.type == TokenType::SHORT_HAND_CLOSE)
            return;

          // We are now outside the tag
          status = Status::OUTSIDE_TAG;
          break;

          // If we found the equals token...
          // Eg.  'Attribute ='
        case TokenType::EQUALS:
          // Indicate that we next need to search for the value
          // for the attribute
          attrib = Attrib::VALUE;
          break;

          // Errors...
        case TokenType::QUOTED_TEXT: /* 'Attribute "InvalidAttr"'*/
        case TokenType::TAG_START: /* 'Attribute <'            */
        case TokenType::TAG_END: /* 'Attribute </'           */
        case TokenType::DECLARATION: /* 'Attribute <?'           */
          throw std::runtime_error("Unexpected token found");
        default:
          break;
        }
        break;

        // If we are looking for an attribute value
      case Attrib::VALUE:
        // Check what the current token type is
        switch (token.type) {
          // If the current type is text or quoted text...
          // Eg.  'Attribute = "Value"' or 'Attribute = Value' or
          // 'Attribute = 'Value''.
        case TokenType::TEXT:
        case TokenType::QUOTED_TEXT:
          // If we are a declaration element '<?' then we need
          // to remove extra closing '?' if it exists
          if (node.IsDeclaration() && token.text.ends_with(_T('?'))) {
            token.text.remove_suffix(1);
          }

          // Add the valued attribute to the list
          if (token.type == TokenType::QUOTED_TEXT) {
            token.text.remove_prefix(1);
            token.text.remove_suffix(1);
          }

          assert(!attribute_name.empty());

          {
            TCHAR *value = FromXMLString(token.text);
            if (value == nullptr)
              throw std::runtime_error("Unexpected token found");

            node.AddAttribute(std::move(attribute_name), value);
            free(value);
          }

          // Indicate we are searching for a new attribute
          attrib = Attrib::NAME;
          break;

          // Errors...
        case TokenType::TAG_START: /* 'Attr = <'          */
        case TokenType::TAG_END: /* 'Attr = </'         */
        case TokenType::CLOSE_TAG: /* 'Attr = >'          */
        case TokenType::SHORT_HAND_CLOSE: /* "Attr = />"         */
        case TokenType::EQUALS: /* 'Attr = ='          */
        case TokenType::DECLARATION: /* 'Attr = <?'         */
          throw std::runtime_error("Unexpected token found");
        default:
          break;
        }
      }
    }
  }
}

/**
 * Parses the given XML String and returns the main XMLNode
 * @param xml_string XML String
 * @param tag (?)
 * @param pResults XMLResult object to write in on error or success
 * @return The main XMLNode or empty XMLNode on error
 */
XMLNode
ParseString(const TCHAR *xml_string)
{
  assert(xml_string != nullptr);

  XMLNode xnode = XMLNode::Null();
  Parser xml{xml_string};

  // Fill the XMLNode xnode with the parsed data of xml
  // note: xnode is now the document node, not the main XMLNode
  ParseXMLElement(xnode, &xml);

  // If the document node does not have childnodes
  XMLNode *child = xnode.GetFirstChild();
  if (child == nullptr)
    throw std::runtime_error("No elements found");

  // Set the document's first childnode as new main node
  xnode = std::move(*child);

  // If the new main node is the xml declaration
  // -> try to take the first childnode again
  if (xnode.IsDeclaration()) {
    // If the declaration does not have childnodes
    child = xnode.GetFirstChild();
    if (child == nullptr)
      throw std::runtime_error("No elements found");

    // Set the declaration's first childnode as new main node
    xnode = std::move(*child);
  }

  // Return the node (empty, main or child of main that equals tag)
  return xnode;
}

static tstring
ReadTextFile(Path path)
{
  /* auto-detect the character encoding, to be able to parse XCSoar
     6.0 task files */
  FileLineReader reader(path, Charset::AUTO);

  long size = reader.GetSize();
  if (size > 65536)
    throw std::runtime_error("File is too large");
  else if (size < 0)
    size = 4096;

  tstring buffer;
  buffer.reserve(size);

  const TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (buffer.length() > 65536)
      /* too long */
      std::runtime_error("File is too large");

    buffer.append(line);
    buffer.append(_T("\n"));
  }

  return buffer;
}

/**
* Opens the file given by the filepath and returns the main node.
* (Includes error handling)
 * @param filename Filepath to the XML file to parse
 * @param tag (?)
 * @return The main XMLNode or an empty node on error
 */
XMLNode
ParseFile(Path filename)
{
  const auto buffer = ReadTextFile(filename);
  return ParseString(buffer.c_str());
}

} // namespace XML
