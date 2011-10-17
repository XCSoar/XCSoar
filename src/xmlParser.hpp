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

#include "Compiler.h"

#include <assert.h>
#include <tchar.h>

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

/** Enumeration used to manage type of data. */
enum XMLElementType {
  eNodeChild = 0,
  eNodeAttribute = 1,
  eNodeText = 2,
  eNodeNULL = 4
};

/** Structure used to obtain error details if the parse fails. */
struct XMLResults {
  enum XMLError error;
  unsigned nLine, nColumn;
};

/** Structure for XML attribute. */
struct XMLAttribute {
  const TCHAR *lpszName;
  const TCHAR *lpszValue;
};

struct XML;

struct XMLNode {
//  friend class XMLNode;
protected:
  /**
   * To allow shallow copy and "intelligent/smart" pointers (automatic
   * delete).
   */
  struct XMLNodeData {
    /** Element name (=NULL if root) */
    const TCHAR *lpszName;

    /** Num of child nodes */
    unsigned nChild;

    /** Num of text fields */
    unsigned nText;

    /** Num of attributes */
    unsigned nAttribute;

    /** Whether node is an XML declaration - '<?xml ?>' */
    bool isDeclaration;

    /** Pointer to parent element (=NULL if root) */
    XMLNode *pParent;

    /** Array of child nodes */
    XMLNode *pChild;

    /** Array of text fields */
    const TCHAR **pText;

    /** Array of attributes */
    XMLAttribute *pAttribute;

    /** order in which the child_nodes,text_fields,clear_fields and */
    unsigned *pOrder;

    unsigned ref_count;
  };

  XMLNodeData *d;

  /**
   * Protected constructor: use "parse" functions to get your first
   * instance of XMLNode.
   */
  XMLNode(XMLNode *pParent, const TCHAR *lpszName, bool isDeclaration);

public:
  // You must create your first instance of XMLNode with these 3 parse functions:
  // (see complete explanation of parameters below)

  static XMLNode createRoot(const TCHAR *lpszName);

  static XMLNode parseString(const TCHAR *lpszXML, XMLResults *pResults=NULL);
  static XMLNode parseFile     (const char *lpszXML, XMLResults *pResults=NULL);
  static XMLNode openFileHelper(const char *lpszXML);

  // The tag parameter should be the name of the first tag inside the XML file.
  // If the tag parameter is omitted, the 3 functions return a node that represents
  // the head of the xml document including the declaration term (<? ... ?>).

  // If the XML document is corrupted:
  //   - The "openFileHelper" method will stop execution and display an error message.
  //   - The 2 other methods will initialize the "pResults" variable with some information that
  //     can be used to trace the error.
  // you can have a detailed explanation of the parsing error with this function:

  static bool GlobalError;

  /**
   * Parse XML errors into a user friendly string.
   */
  static const TCHAR *getError(XMLError error);

  /**
   * name of the node
   */
  const TCHAR *getName() const {
    assert(d != NULL);

    return d->lpszName;
  }

  /** @return ith text field */
  const TCHAR *getText(unsigned i = 0) const;

  /** nbr of text field */
  unsigned nText() const {
    assert(d != NULL);

    return d->nText;
  }

  /** @return ith child node */
  XMLNode getChildNode(unsigned i);

  /**
   * @return ith child node with specific name (return an empty node
   * if failing)
   */
  XMLNode getChildNode(const TCHAR *name, unsigned i);

  /**
   * @return next child node with specific name (return an empty node
   * if failing)
   */
  XMLNode getChildNode(const TCHAR *name, unsigned *i=NULL);

  /** @return the number of child node with specific name */
  unsigned nChildNode(const TCHAR *name) const;

  /** nbr of child node */
  unsigned nChildNode() const;

  /** @return ith attribute */
  XMLAttribute getAttribute(unsigned i);

  /** test if an attribute with a specific name is given */
  bool isAttributeSet(const TCHAR *name) const;

  /**
   * @return ith attribute content with specific name (return a NULL
   * if failing)
   */
  const TCHAR *getAttribute(const TCHAR *name, unsigned i) const;

  /**
   * @return next attribute content with specific name (return a NULL
   * if failing)
   */
  const TCHAR *getAttribute(const TCHAR *name, unsigned *i=NULL) const;

  /**
   * nbr of attribute
   */
  unsigned nAttribute() const {
    assert(d != NULL);

    return d->nAttribute;
  }

  /**
   * Create an XML file from the head element.
   *
   * @param writer the stream to write the XML text to

   * @param nFormat 0 if no formatting is required, otherwise nonzero
   * for formatted text with carriage returns and indentation.
   */
  void serialise(TextWriter &writer, int nFormat) const;

  /**
   * nbr of different contents for current node
   */
  gcc_pure
  unsigned nElement() const {
    assert(d != NULL);

    return d->nChild + d->nText + d->nAttribute;
  }

  /** is this node empty? */
  gcc_pure
  bool isEmpty() const {
    return d == NULL;
  }

  gcc_pure
  bool isDeclaration() const {
    assert(d != NULL);

    return d->isDeclaration;
  }

  // to allow shallow copy:
  ~XMLNode();

  /**
   * Shallow copy.
   */
  XMLNode(const XMLNode &A);

  /**
   * Shallow copy.
   */
  XMLNode& operator=(const XMLNode& A);

  static void destroyCurrentBuffer(XMLNodeData *d);

  gcc_constexpr_ctor
  XMLNode(): d(NULL) {}

  // The strings given as parameters for these 4 methods will be free'd by the XMLNode class:

  /**
   * Add a child node to the given element.
   */
  XMLNode AddChild(const TCHAR *lpszName, bool isDeclaration);

  /**
   * Add an attribute to an element.
   */
  void AddAttribute(const TCHAR *lpszName, const TCHAR *lpszValuev);

  /**
   * Add text to the element.
   */
  const TCHAR *AddText(const TCHAR *lpszValue);

private:
  // these are functions used internally (don't bother about them):
  bool ParseXMLElement(XML *pXML);
  void addToOrder(unsigned index, unsigned type);

  /**
   * Creates an user friendly XML string from a given element with
   * appropriate white space and carriage returns.
   *
   * This recurses through all subnodes then adds contents of the
   * nodes to the string.
   */
  static void serialiseR(const XMLNodeData *pEntry, TextWriter &writer,
                         int nFormat);
  static const void *enumContent(const XMLNodeData *pEntry, unsigned i,
                                 XMLElementType *nodeType);

  gcc_pure
  static unsigned nElement(const XMLNodeData *pEntry);

  /**
   * Update "order" information when deleting a content of a XMLNode.
   */
  static void removeOrderElement(XMLNodeData *d, XMLElementType t,
                                 unsigned index);
};

#endif
