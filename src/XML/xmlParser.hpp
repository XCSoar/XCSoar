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
#ifndef __INCLUDE_XML_NODE__
#define __INCLUDE_XML_NODE__

#include "Util/NonCopyable.hpp"
#include "Util/tstring.hpp"
#include "Compiler.h"

#include <list>
#include <forward_list>

#include <assert.h>
#include <tchar.h>
#include <stdlib.h>

class TextWriter;

/** Enumeration for XML parse errors. */
enum XMLError {
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
struct XMLResults {
  enum XMLError error;
  unsigned line, column;
};

struct XML;

struct XMLNode {
//  friend class XMLNode;
protected:
  /**
   * To allow shallow copy and "intelligent/smart" pointers (automatic
   * delete).
   */
  struct Data : private NonCopyable {
    /** Structure for XML attribute. */
    struct Attribute : private NonCopyable {
      TCHAR *name;
      TCHAR *value;

      Attribute(TCHAR *_name, TCHAR *_value)
        :name(_name), value(_value) {}

      ~Attribute() {
        free(name);
        free(value);
      }
    };

    /** Element name (=NULL if root) */
    const TCHAR *name;

    /** Whether node is an XML declaration - '<?xml ?>' */
    bool is_declaration;

    /** Array of child nodes */
    std::list<XMLNode> children;

    /** A concatentation of all text nodes */
    tstring text;

    /** Array of attributes */
    std::forward_list<Attribute> attributes;

    unsigned ref_count;

    Data(const TCHAR *_name, bool _is_declaration)
      :name(_name),
       is_declaration(_is_declaration),
       ref_count(1) {}
    ~Data();

    void Ref();
    void Unref();

    bool HasChildren() const {
      return !children.empty() || !text.empty();
    }

    void AddAttribute(TCHAR *name, TCHAR *value) {
      attributes.emplace_front(name, value);
    }

    typedef std::list<XMLNode>::const_iterator const_iterator;

    const_iterator begin() const {
      return children.begin();
    }

    const_iterator end() const {
      return children.end();
    }
  };

  Data *d;

  /**
   * Protected constructor: use "parse" functions to get your first
   * instance of XMLNode.
   */
  XMLNode(const TCHAR *name, bool is_declaration);

public:
  // You must create your first instance of XMLNode with these 3 parse functions:
  // (see complete explanation of parameters below)

  static XMLNode CreateRoot(const TCHAR *name);

  static XMLNode *ParseString(const TCHAR *xml_string,
                              XMLResults *pResults=NULL);
  static XMLNode *ParseFile(const TCHAR *path, XMLResults *pResults=NULL);
  static XMLNode *OpenFileHelper(const TCHAR *path);

  // The tag parameter should be the name of the first tag inside the XML file.
  // If the tag parameter is omitted, the 3 functions return a node that represents
  // the head of the xml document including the declaration term (<? ... ?>).

  // If the XML document is corrupted:
  //   - The "openFileHelper" method will stop execution and display an error message.
  //   - The 2 other methods will initialize the "pResults" variable with some information that
  //     can be used to trace the error.
  // you can have a detailed explanation of the parsing error with this function:

  static bool global_error;

  /**
   * Parse XML errors into a user friendly string.
   */
  gcc_const
  static const TCHAR *GetErrorMessage(XMLError error);

  /**
   * name of the node
   */
  const TCHAR *GetName() const {
    assert(d != NULL);

    return d->name;
  }

  typedef Data::const_iterator const_iterator;

  const_iterator begin() const {
    return d->begin();
  }

  const_iterator end() const {
    return d->end();
  }

  /**
   * @return the first child node, or NULL if there is none
   */
  gcc_pure
  const XMLNode *GetFirstChild() const {
    return d != NULL && !d->children.empty()
      ? &d->children.front()
      : NULL;
  }

private:
  /**
   * @return the first child node, or NULL if there is none
   */
  gcc_pure
  XMLNode *GetFirstChild() {
    return d != NULL && !d->children.empty()
      ? &d->children.front()
      : NULL;
  }

public:
  /**
   * @return ith child node with specific name (return an empty node
   * if failing)
   */
  gcc_pure
  const XMLNode *GetChildNode(const TCHAR *name) const;

  /**
   * @return ith attribute content with specific name (return a NULL
   * if failing)
   */
  gcc_pure
  const TCHAR *GetAttribute(const TCHAR *name) const;

  /**
   * Create an XML file from the head element.
   *
   * @param writer the stream to write the XML text to

   * @param format 0 if no formatting is required, otherwise nonzero
   * for formatted text with carriage returns and indentation.
   */
  void Serialise(TextWriter &writer, int format) const;

  gcc_pure
  bool IsDeclaration() const {
    assert(d != NULL);

    return d->is_declaration;
  }

  // to allow shallow copy:
  ~XMLNode() {
    if (d != NULL)
      d->Unref();
  }

  /**
   * Shallow copy.
   */
  XMLNode(const XMLNode &A);

  XMLNode(XMLNode &&other)
    :d(other.d) {
    other.d = NULL;
  }

  /**
   * Shallow copy.
   */
  XMLNode &operator=(const XMLNode& A);

  XMLNode &operator=(XMLNode &&other) {
    std::swap(d, other.d);
    return *this;
  }

  gcc_constexpr_ctor
  XMLNode(): d(NULL) {}

  // The strings given as parameters for these 4 methods will be free'd by the XMLNode class:

  /**
   * Add a child node to the given element.
   */
  XMLNode &AddChild(const TCHAR *name, bool is_declaration);

  /**
   * Add an attribute to an element.
   */
  void AddAttribute(TCHAR *name, TCHAR *value);

  /**
   * Add text to the element.
   */
  void AddText(const TCHAR *value);

  void AddText(const TCHAR *text, size_t length);

private:
  // these are functions used internally (don't bother about them):
  bool ParseXMLElement(XML *pXML);

  /**
   * Creates an user friendly XML string from a given element with
   * appropriate white space and carriage returns.
   *
   * This recurses through all subnodes then adds contents of the
   * nodes to the string.
   */
  static void Serialise(const Data &data, TextWriter &writer, int format);
};

#endif
