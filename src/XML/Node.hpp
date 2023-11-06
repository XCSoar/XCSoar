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

#pragma once

#include <list>
#include <forward_list>
#include <string>
#include <string_view>

class BufferedOutputStream;

class XMLNode {
  /** Structure for XML attribute. */
  struct Attribute {
    std::string name;
    std::string value;

    template<typename N, typename V>
    Attribute(N &&name, V &&value) noexcept
      :name(std::forward<N>(name)), value(std::forward<V>(value)) {}
  };

  /** Element name (=nullptr if root) */
  std::string name;

  /** Whether node is an XML declaration - '<?xml ?>' */
  bool is_declaration;

  /** Array of child nodes */
  std::list<XMLNode> children;

  /** A concatentation of all text nodes */
  std::string text;

  /** Array of attributes */
  std::forward_list<Attribute> attributes;

  /**
   * Protected constructor: use "parse" functions to get your first
   * instance of XMLNode.
   */
  XMLNode(std::string_view name,
          bool is_declaration) noexcept;

public:
  static inline XMLNode Null() noexcept {
    return XMLNode({}, false);
  }

  static XMLNode CreateRoot(const char *name) noexcept;

  bool IsNull() const noexcept {
    return name.empty();
  }

  /**
   * name of the node
   */
  const char *GetName() const noexcept {
    return name.c_str();
  }

  using const_iterator = std::list<XMLNode>::const_iterator;

  const_iterator begin() const noexcept {
    return children.begin();
  }

  const_iterator end() const noexcept {
    return children.end();
  }

  bool HasChildren() const noexcept {
    return !children.empty() || !text.empty();
  }

  /**
   * @return the first child node, or nullptr if there is none
   */
  [[gnu::pure]]
  const XMLNode *GetFirstChild() const noexcept {
    return !children.empty()
      ? &children.front()
      : nullptr;
  }

  /**
   * @return the first child node, or nullptr if there is none
   */
  [[gnu::pure]]
  XMLNode *GetFirstChild() noexcept {
    return !children.empty()
      ? &children.front()
      : nullptr;
  }

public:
  /**
   * @return ith child node with specific name (return an empty node
   * if failing)
   */
  [[gnu::pure]]
  const XMLNode *GetChildNode(const char *name) const noexcept;

  /**
   * @return ith attribute content with specific name (return a nullptr
   * if failing)
   */
  [[gnu::pure]]
  const char *GetAttribute(const char *name) const noexcept;

  /**
   * Create an XML file from the head element.
   *
   * @param writer the stream to write the XML text to

   * @param format false if no formatting is required, true
   * for formatted text with carriage returns and indentation.
   */
  void Serialise(BufferedOutputStream &os, bool format) const;

  [[gnu::pure]]
  bool IsDeclaration() const noexcept {
    return is_declaration;
  }

  XMLNode(const XMLNode &A) = delete;

  XMLNode(XMLNode &&other) noexcept = default;

  /**
   * Shallow copy.
   */
  XMLNode &operator=(const XMLNode& A) = delete;

  XMLNode &operator=(XMLNode &&other) noexcept = default;

  /**
   * Add a child node to the given element.
   */
  XMLNode &AddChild(const std::string_view name,
                    bool is_declaration=false) noexcept;

  /**
   * Add an attribute to an element.
   */
  template<typename N, typename V>
  void AddAttribute(N &&name, V &&value) noexcept {
    attributes.emplace_front(std::forward<N>(name), std::forward<V>(value));
  }

  /**
   * Add text to the element.
   */
  void AddText(std::string_view value) noexcept {
    text.append(value);
  }

private:
  /**
   * Creates an user friendly XML string from a given element with
   * appropriate white space and carriage returns.
   *
   * This recurses through all subnodes then adds contents of the
   * nodes to the string.
   */
  void SerialiseInner(BufferedOutputStream &os, int format) const;
};
