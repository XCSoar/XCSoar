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

#ifndef XCSOAR_XML_NODE_HPP
#define XCSOAR_XML_NODE_HPP

#include "Util/NonCopyable.hpp"
#include "Util/tstring.hpp"
#include "Compiler.h"

#include <list>
#include <forward_list>

#include <assert.h>
#include <tchar.h>

class BufferedOutputStream;

class XMLNode {
  /**
   * To allow shallow copy and "intelligent/smart" pointers (automatic
   * delete).
   */
  struct Data : private NonCopyable {
    /** Structure for XML attribute. */
    struct Attribute : private NonCopyable {
      tstring name, value;

      Attribute(tstring &&_name, const TCHAR *_value, size_t value_length)
        :name(std::move(_name)), value(_value, value_length) {}

      Attribute(const TCHAR *_name, const TCHAR *_value)
        :name(_name), value(_value) {}

      Attribute(const TCHAR *_name, size_t name_length,
                const TCHAR *_value, size_t value_length)
        :name(_name, name_length), value(_value, value_length) {}
    };

    /** Element name (=nullptr if root) */
    tstring name;

    /** Whether node is an XML declaration - '<?xml ?>' */
    bool is_declaration;

    /** Array of child nodes */
    std::list<XMLNode> children;

    /** A concatentation of all text nodes */
    tstring text;

    /** Array of attributes */
    std::forward_list<Attribute> attributes;

    Data(const TCHAR *_name, bool _is_declaration)
      :name(_name),
       is_declaration(_is_declaration) {}

    Data(const TCHAR *_name, size_t name_length, bool _is_declaration)
      :name(_name, name_length),
       is_declaration(_is_declaration) {}

    bool HasChildren() const {
      return !children.empty() || !text.empty();
    }

    void AddAttribute(tstring &&name,
                      const TCHAR *value, size_t value_length) {
      attributes.emplace_front(std::move(name), value, value_length);
    }

    void AddAttribute(const TCHAR *name, const TCHAR *value) {
      attributes.emplace_front(name, value);
    }

    void AddAttribute(const TCHAR *name, size_t name_length,
                      const TCHAR *value, size_t value_length) {
      attributes.emplace_front(name, name_length, value, value_length);
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

  XMLNode(const TCHAR *name, size_t name_length, bool is_declaration);

public:
  static inline XMLNode Null() {
    return XMLNode(_T(""), false);
  }

  static XMLNode CreateRoot(const TCHAR *name);

  /**
   * name of the node
   */
  const TCHAR *GetName() const {
    assert(d != nullptr);

    return d->name.c_str();
  }

  typedef Data::const_iterator const_iterator;

  const_iterator begin() const {
    return d->begin();
  }

  const_iterator end() const {
    return d->end();
  }

  /**
   * @return the first child node, or nullptr if there is none
   */
  gcc_pure
  const XMLNode *GetFirstChild() const {
    return d != nullptr && !d->children.empty()
      ? &d->children.front()
      : nullptr;
  }

  /**
   * @return the first child node, or nullptr if there is none
   */
  gcc_pure
  XMLNode *GetFirstChild() {
    return d != nullptr && !d->children.empty()
      ? &d->children.front()
      : nullptr;
  }

public:
  /**
   * @return ith child node with specific name (return an empty node
   * if failing)
   */
  gcc_pure
  const XMLNode *GetChildNode(const TCHAR *name) const;

  /**
   * @return ith attribute content with specific name (return a nullptr
   * if failing)
   */
  gcc_pure
  const TCHAR *GetAttribute(const TCHAR *name) const;

  /**
   * Create an XML file from the head element.
   *
   * @param writer the stream to write the XML text to

   * @param format false if no formatting is required, true
   * for formatted text with carriage returns and indentation.
   */
  void Serialise(BufferedOutputStream &os, bool format) const;

  gcc_pure
  bool IsDeclaration() const {
    assert(d != nullptr);

    return d->is_declaration;
  }

  // to allow shallow copy:
  ~XMLNode() {
    delete d;
  }

  XMLNode(const XMLNode &A) = delete;

  XMLNode(XMLNode &&other)
    :d(other.d) {
    other.d = nullptr;
  }

  /**
   * Shallow copy.
   */
  XMLNode &operator=(const XMLNode& A) = delete;

  XMLNode &operator=(XMLNode &&other) {
    Data *old = d;
    d = other.d;
    other.d = nullptr;

    delete old;

    return *this;
  }

  constexpr
  XMLNode(): d(nullptr) {}

  // The strings given as parameters for these 4 methods will be free'd by the XMLNode class:

  /**
   * Add a child node to the given element.
   */
  XMLNode &AddChild(const TCHAR *name, bool is_declaration=false);

  XMLNode &AddChild(const TCHAR *name, size_t name_length,
                    bool is_declaration=false);

  /**
   * Add an attribute to an element.
   */
  void AddAttribute(tstring &&name,
                    const TCHAR *value, size_t value_length) {
    d->AddAttribute(std::move(name), value, value_length);
  }

  void AddAttribute(const TCHAR *name, const TCHAR *value) {
    assert(name != nullptr);
    assert(value != nullptr);

    d->AddAttribute(name, value);
  }

  void AddAttribute(const TCHAR *name, size_t name_length,
                    const TCHAR *value, size_t value_length) {
    assert(name != nullptr);
    assert(value != nullptr);

    d->AddAttribute(name, name_length, value, value_length);
  }

  /**
   * Add text to the element.
   */
  void AddText(const TCHAR *value);

  void AddText(const TCHAR *text, size_t length);

private:
  /**
   * Creates an user friendly XML string from a given element with
   * appropriate white space and carriage returns.
   *
   * This recurses through all subnodes then adds contents of the
   * nodes to the string.
   */
  static void Serialise(const Data &data, BufferedOutputStream &os,
                        int format);
};

#endif
