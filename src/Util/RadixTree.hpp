/*
 * Copyright (C) 2010 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_RADIX_TREE_HPP
#define XCSOAR_RADIX_TREE_HPP

#include "Util/NonCopyable.hpp"
#include "Util/StaticString.hpp"
#include "StringUtil.hpp"
#include "tstring.hpp"

#include <algorithm>
#include <assert.h>

#ifdef PRINT_RADIX_TREE
#include <ostream>
#endif

/**
 * An associative container which maps TCHAR strings to arbitrary
 * objects.  Each "key" may have multiple values.
 *
 * This class provides a sorted iterator, which is optimized for
 * prefix search.  This is useful e.g. when you want to show an
 * incremental result set when the user starts typing an item name.
 * Besides that, a key lookup is quite efficient.
 */
template<typename T>
class RadixTree {
  template<class V>
  struct KeyVisitorAdapter {
    V &visitor;
    const TCHAR *key;

    KeyVisitorAdapter(V &_visitor, const TCHAR *_key)
      :visitor(_visitor), key(_key) {}

    void operator()(const T &value) const {
      visitor(key, value);
    }
  };

  /**
   * A leaf holds one value associated with a key.  Next to the value,
   * it has a "next" attribute to build a singly linked list.
   */
  struct Leaf {
    Leaf *next;
    T value;

    Leaf(Leaf *_next, const T &_value)
      :next(_next), value(_value) {}
  };

  /**
   * A linked list of Leaf objects.
   */
  struct LeafList {
    Leaf *head;

    gcc_constexpr_ctor
    LeafList():head(NULL) {}

    ~LeafList() {
      Leaf *next = head;
      while (next != NULL) {
        Leaf *leaf = next;
        next = leaf->next;

        delete leaf;
      }
    }

    void Clear() {
      Leaf *next = head;
      while (next != NULL) {
        Leaf *leaf = next;
        next = leaf->next;

        delete leaf;
      }

      head = NULL;
    }

    void Swap(LeafList &other) {
      std::swap(head, other.head);
    }

    void Add(const T &value) {
      head = new Leaf(head, value);
    }

    bool Remove(const T &value) {
      Leaf **leaf_r = &head;

      while (*leaf_r != NULL) {
        Leaf *leaf = *leaf_r;
        if (leaf->value == value) {
          *leaf_r = leaf->next;
          leaf->next = NULL;
          delete leaf;
          return true;
        }

        leaf_r = &leaf->next;
      }

      return false;
    }

    T *GetFirstPointer() {
      return head != NULL
        ? &head->value
        : NULL;
    }

    const T *GetFirstPointer() const {
      return head != NULL
        ? &head->value
        : NULL;
    }

    template<class P>
    T *GetIf(const P &predicate) {
        for (Leaf *leaf = head; leaf != NULL; leaf = leaf->next)
          if (predicate(leaf->value))
            return &leaf->value;

        return NULL;
    }

    template<class P>
    const T *GetIf(const P &predicate) const {
        for (Leaf *leaf = head; leaf != NULL; leaf = leaf->next)
          if (predicate(leaf->value))
            return &leaf->value;

        return NULL;
    }

    template<typename V>
    void VisitAll(V &visitor) {
      for (Leaf *leaf = head; leaf != NULL; leaf = leaf->next)
        visitor(leaf->value);
    }

    template<typename V>
    void VisitAll(V &visitor) const {
      for (const Leaf *leaf = head; leaf != NULL; leaf = leaf->next)
        visitor(leaf->value);
    }
  };

  /**
   * A node in the radix tree.  The "label" attribute is a substring
   * of the key.  A node can have any number of values (or none).
   *
   * All siblings start with a different letter (this is a basic
   * property of radix trees).  When inserting a node starting with
   * the same character, the node has to be splitted.
   */
  struct Node : private NonCopyable {
    StaticString<8> label;
    Node *next_sibling, *children;
    LeafList leaves;

    gcc_constexpr_ctor
    Node(const TCHAR *_label)
      :label(_label),
       next_sibling(NULL), children(NULL) {}
    ~Node() {
      delete next_sibling;
      delete children;
    }

    /**
     * Create a new Node with the specified label (suffix) and value.
     * If the label is too long for the StaticString, multiple chained
     * Node objects are created.
     */
    Node *CreateLeaf(const TCHAR *label, const T &value) const {
      Node *top = new Node(label), *bottom = top;
      while (_tcslen(label) >= Node::label.MAX_SIZE) {
        /* label too long for the Node's StaticString, create another
           child Node */
        label += Node::label.MAX_SIZE - 1;
        Node *node = new Node(label);
        bottom->children = node;
        bottom = node;
      }

      bottom->add_value(value);
      return top;
    }

    void clear() {
      delete children;
      children = NULL;
      leaves.Clear();
    }

    /**
     * Find the first mismatching character.  Returns the original
     * parameter if there is no match at all.  Returns the end of the
     * string if there is a full match (even if the node's label is
     * longer).
     */
    const TCHAR *match(const TCHAR *key) const {
      const TCHAR *l = label.c_str();

      while (!string_is_empty(key) && *key == *l) {
        ++key;
        ++l;
      }

      return key;
    }

    /**
     * Check if the label begins with the specified prefix.  Returns
     * the original parameter if there is no match at all.  Returns
     * the end of the prefix string if there is a full match (even if
     * the node's label is longer).  If the label is shorter than the
     * prefix, returns a pointer to the first prefix character which
     * is outside of the label's scope.  Returns NULL if there is a
     * mismatch.
     */
    const TCHAR *match_prefix(const TCHAR *prefix) const {
      const TCHAR *l = label.c_str();

      while (!string_is_empty(prefix) && !string_is_empty(l)) {
        if (*l != *prefix)
          return NULL;

        ++prefix;
        ++l;
      }

      return prefix;
    }

    T *get(const TCHAR *key) {
      if (string_is_empty(key))
        /* found */
        return leaves.GetFirstPointer();

      Match m = find_child(key);
      return m.is_full_match(key)
        ? m.node->get(m.key)
        : NULL;
    }

    const T *get(const TCHAR *key) const {
      if (string_is_empty(key))
        /* found */
        return leaves.GetFirstPointer();

      Match m = find_child(key);
      return m.is_full_match(key)
        ? m.node->get(m.key)
        : NULL;
    }

    template<class P>
    T *GetIf(const TCHAR *key, const P &predicate) {
      if (string_is_empty(key))
        /* found */
        return leaves.GetIf(predicate);

      Match m = find_child(key);
      return m.is_full_match(key)
        ? m.node->GetIf(m.key, predicate)
        : NULL;
    }

    template<class P>
    const T *GetIf(const TCHAR *key, const P &predicate) const {
      if (string_is_empty(key))
        /* found */
        return leaves.GetIf(predicate);

      Match m = find_child(key);
      return m.is_full_match(key)
        ? m.node->GetIf(m.key, predicate)
        : NULL;
    }

    TCHAR *suggest(const TCHAR *prefix, TCHAR *dest, size_t max_length) const {
      if (string_is_empty(prefix)) {
        /* exact match - return the first character of all child
           nodes */
        TCHAR *retval = dest, *end = dest + max_length - 1;

        for (const Node *node = children; node != NULL && dest < end;
             node = node->next_sibling)
          *dest++ = node->label[0u];

        *dest = _T('\0');
        return retval;
      }

      Match m = find_child(prefix);
      if (m.key == prefix)
        /* mismatch */
        return NULL;

      if (m.is_full_match(prefix))
        /* recurse */
        return m.node->suggest(m.key, dest, max_length);

      /* return one character */
      dest[0u] = m.node->label[(unsigned)(m.key - prefix)];
      dest[1] = _T('\0');
      return dest;
    }

    /**
     * Split the node label at the specified position, creating a new
     * child node which becomes the parent of all this node's children
     * and values.
     */
    void split(size_t length) {
      assert(length > 0);
      assert(length < label.length());

      Node *node = new Node(label.c_str() + length);
      node->children = children;
      children = node;

      leaves.Swap(node->leaves);

      label.Truncate(length);
    }

    /**
     * Add a value.  There is no specific order of values with the
     * same key.
     */
    void add_value(const T &value) {
      leaves.Add(value);
    }

    /**
     * Remove all values of this node.
     */
    void remove_values() {
      leaves.Clear();
    }

    /**
     * Remove the specified value.  If there are multiple instances of
     * the same value, only one is removed.
     *
     * @return true if a value was found and removed
     */
    bool remove_value(const T &value) {
      return leaves.Remove(value);
    }

    /**
     * Remove all values with the specified key.
     */
    void remove_values(const TCHAR *key) {
      assert(key != NULL);

      if (string_is_empty(key)) {
        /* this is the right node */
        remove_values();
      } else {
        Match m = find_child(key);
        if (m.is_full_match(key))
          /* recurse */
          m.node->remove_values(m.key);
      }
    }

    /**
     * Remove the specified value.
     *
     * @return true if a value was found and removed
     */
    bool remove_value(const TCHAR *key, const T &value) {
      assert(key != NULL);

      if (string_is_empty(key)) {
        /* this is the right node */
        return remove_value(value);
      } else {
        Match m = find_child(key);
        return m.is_full_match(key) &&
          m.node->remove_value(m.key, value);
      }
    }

    /**
     * Visit all direct values of this node in no specific order.
     */
    template<typename V>
    void visit_values(V &visitor) {
      leaves.VisitAll(visitor);
    }

    /**
     * Visit all direct values of this node in no specific order.
     */
    template<typename V>
    void visit_values(V &visitor) const {
      leaves.VisitAll(visitor);
    }

    /**
     * Visit all direct values of this node in no specific order.
     * This overload is used by visit_all_pairs().
     */
    template<typename V>
    void visit_values(const TCHAR *prefix, V &visitor) const {
      tstring key(prefix);
      key.append(label);

      const KeyVisitorAdapter<V> adapter(visitor, key.c_str());
      visit_values(adapter);
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.
     */
    template<typename V>
    void visit_all_children(V &visitor) {
      for (Node *node = children; node != NULL; node = node->next_sibling) {
        node->visit_values(visitor);
        node->visit_all_children(visitor);
      }
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.
     */
    template<typename V>
    void visit_all_children(V &visitor) const {
      for (const Node *node = children; node != NULL; node = node->next_sibling) {
        node->visit_values(visitor);
        node->visit_all_children(visitor);
      }
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.  This overload is used by visit_all_pairs().
     */
    template<typename V>
    void visit_all_children(const TCHAR *prefix, V &visitor) const {
      tstring key(prefix);
      key.append(label);

      for (const Node *node = children; node != NULL; node = node->next_sibling) {
        node->visit_values(key.c_str(), visitor);
        node->visit_all_children(key.c_str(), visitor);
      }
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void visit_children(const TCHAR *key, V &visitor) {
      visit_siblings(children, key, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void visit_children(const TCHAR *key, V &visitor) const {
      visit_siblings(const_cast<const Node *>(children), key, visitor);
    }

    /**
     * Recursively visit all values with the specified key.  The
     * key is matched on this node's label first.
     */
    template<typename V>
    void visit(const TCHAR *key, V &visitor) {
      const TCHAR *match = match_prefix(key);
      if (match == NULL)
        return;

      if (string_is_empty(match))
        visit_values(visitor);
      else
        visit_children(match, visitor);
    }

    /**
     * Recursively visit all values with the specified key.  The
     * key is matched on this node's label first.
     */
    template<typename V>
    void visit(const TCHAR *key, V &visitor) const {
      const TCHAR *match = match_prefix(key);
      if (match == NULL)
        return;

      if (string_is_empty(match))
        visit_values(visitor);
      else
        visit_children(match, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void visit_prefix_children(const TCHAR *prefix, V &visitor) {
      if (string_is_empty(prefix))
        visit_all_children(visitor);
      else
        for (Node *node = children; node != NULL; node = node->next_sibling)
          node->visit_prefix(prefix, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void visit_prefix_children(const TCHAR *prefix, V &visitor) const {
      if (string_is_empty(prefix))
        visit_all_children(visitor);
      else
        for (const Node *node = children; node != NULL; node = node->next_sibling)
          node->visit_prefix(prefix, visitor);
    }

    /**
     * Recursively visit all values and child nodes with the specified
     * prefix in alphabetic order.  The prefix is matched on this
     * node's label first.
     */
    template<typename V>
    void visit_prefix(const TCHAR *prefix, V &visitor) {
      const TCHAR *match = match_prefix(prefix);
      if (match == NULL)
        return;

      if (string_is_empty(match)) {
        visit_values(visitor);
        visit_all_children(visitor);
      } else {
        visit_prefix_children(match, visitor);
      }
    }

    /**
     * Recursively visit all values and child nodes with the specified
     * prefix in alphabetic order.  The prefix is matched on this
     * node's label first.
     */
    template<typename V>
    void visit_prefix(const TCHAR *prefix, V &visitor) const {
      const TCHAR *match = match_prefix(prefix);
      if (match == NULL)
        return;

      if (string_is_empty(match)) {
        visit_values(visitor);
        visit_all_children(visitor);
      } else {
        visit_prefix_children(match, visitor);
      }
    }

    struct Match {
      Node *node;
      const TCHAR *key;

      Match(Node *_node, const TCHAR *_key)
        :node(_node), key(_key) {}

      bool is_full_match(const TCHAR *key) {
        return this->key != key && this->key >= key + node->label.length();
      }
    };

    /**
     * Find a matching child node.  Returns a pair containing the node
     * pointer (or NULL if this node has no children), and a pointer
     * to the portion of the key which was not used yet.
     */
    Match find_child(const TCHAR *key) const {
      Node *node = children, *prev = NULL;
      while (node != NULL) {
        const TCHAR *label = node->label.c_str();
        if (key[0u] < label[0u])
          return Match(prev, key);
        else if (key[0u] == label[0u])
          return Match(node, node->match(key));

        prev = node;
        node = node->next_sibling;
      }

      return Match(prev, key);
    }

    /**
     * Adds a new value relative to this node, possibly creating a new
     * node and/or splitting an existing node.
     */
    void add(const TCHAR *key, const T &value) {
      assert(key != NULL);

      if (string_is_empty(key)) {
        /* add to this node */
        add_value(value);
        return;
      }

      Match m = find_child(key);
      if (m.key == key) {
        /* no match - create new node */
        Node *node = CreateLeaf(key, value);

        if (m.node == NULL) {
          /* insert before list head */
          node->next_sibling = children;
          children = node;
        } else {
          /* insert after that node */
          node->next_sibling = m.node->next_sibling;
          m.node->next_sibling = node;
        }
      } else if (m.is_full_match(key)) {
        m.node->add(m.key, value);
      } else {
        /* split existing node */
        m.node->split(m.key - key);

        if (string_is_empty(m.key)) {
          /* add to splitted parent node */
          m.node->add_value(value);
        } else {
          Node *node = CreateLeaf(m.key, value);

          if (m.key[0u] < m.node->children->label[0u]) {
            /* insert before list head */
            node->next_sibling = m.node->children;
            m.node->children = node;
          } else {
            /* insert after the splitted child node */
            assert(m.node->children->next_sibling == NULL);

            m.node->children->next_sibling = node;
          }
        }
      }
    }

#ifdef PRINT_RADIX_TREE
    template <typename Char, typename Traits>
    friend std::basic_ostream<Char, Traits> &
    operator<<(typename std::basic_ostream<Char, Traits>& out,
               const Node &node) {
      out << "node '" << node.label << "' {\n";
      for (const RadixTree<T>::Leaf *leaf = node.leaves; leaf != NULL;
           leaf = leaf->next)
        out << "  value " << leaf->value << "\n";
      for (const RadixTree<T>::Node *child = node.children; child != NULL;
           child = child->next_sibling)
        out << *child;
      return out << "}\n";
    }
#endif /* PRINT_RADIX_TREE */
  };

  /**
   * The root node is a special case: its key is the empty string, and
   * it is never split, it has no siblings.
   */
  Node root;

public:
  gcc_constexpr_ctor
  RadixTree():root(_T("")) {}

  /**
   * Gets a value for the specified key.  Returns the parameter
   * default_value if the specified key is not present.  If there are
   * multiple values, any one is returned.
   */
  T &get(const TCHAR *key, T &default_value) {
    T *value = root.get(key);
    return value != NULL
      ? *value
      : default_value;
  }

  /**
   * Gets a value for the specified key.  Returns the parameter
   * default_value if the specified key is not present.  If there are
   * multiple values, any one is returned.
   */
  const T &get(const TCHAR *key, const T &default_value) const {
    const T *value = root.get(key);
    return value != NULL
      ? *value
      : default_value;
  }

  template<class P>
  T &GetIf(const TCHAR *key, T &default_value, const P &predicate) {
    const T *value = root.GetIf(key, predicate);
    return value != NULL
      ? *value
      : default_value;
  }

  template<class P>
  const T &GetIf(const TCHAR *key, const T &default_value,
                 const P &predicate) const {
    const T *value = root.GetIf(key, predicate);
    return value != NULL
      ? *value
      : default_value;
  }

  /**
   * Get a list of characters following the specified prefix.  There
   * is no special indication whether there is a value for the exact
   * key.
   *
   * @param prefix the prefix
   * @param dest the destination buffer which will be filled with
   * characters
   * @param max_length the size of the buffer, including the trailing
   * null byte
   * @return the destination buffer, or NULL if the prefix does not
   * occur in the tree
   */
  TCHAR *suggest(const TCHAR *prefix, TCHAR *dest, size_t max_length) const {
    return root.suggest(prefix, dest, max_length);
  }

  void clear() {
    root.clear();
  }

  /**
   * Add a new value with the specified key.  Multiple values can
   * exist for one key.
   */
  void add(const TCHAR *key, const T &value) {
    root.add(key, value);
  }

  /**
   * Remove all values with the specified key.
   */
  void remove(const TCHAR *key) {
    assert(key != NULL);

    root.remove_values(key);
  }

  /**
   * Remove a value with the specified key.
     *
     * @return true if a value was found and removed
   */
  bool remove(const TCHAR *key, const T &value) {
    assert(key != NULL);

    return root.remove_value(key, value);
  }

  /**
   * Visit all values in alphabetic order.
   */
  template<typename V>
  void visit_all(V &visitor) {
    root.visit_values(visitor);
    root.visit_all_children(visitor);
  }

  /**
   * Visit all values in alphabetic order.
   */
  template<typename V>
  void visit_all(V &visitor) const {
    root.visit_values(visitor);
    root.visit_all_children(visitor);
  }

  /**
   * Visit all key/value pairs in alphabetic order.
   */
  template<typename V>
  void visit_all_pairs(V &visitor) const {
    root.visit_values(_T(""), visitor);
    root.visit_all_children(_T(""), visitor);
  }

  /**
   * Visit all values with the specified key.
   */
  template<typename V>
  void visit(const TCHAR *key, V &visitor) {
    root.visit(key, visitor);
  }

  /**
   * Visit all values with the specified key.
   */
  template<typename V>
  void visit(const TCHAR *key, V &visitor) const {
    root.visit(key, visitor);
  }

  /**
   * Visit all values matching the specified prefix in alphabetic
   * order.
   */
  template<typename V>
  void visit_prefix(const TCHAR *prefix, V &visitor) {
    root.visit_prefix(prefix, visitor);
  }

  /**
   * Visit all values matching the specified prefix in alphabetic
   * order.
   */
  template<typename V>
  void visit_prefix(const TCHAR *prefix, V &visitor) const {
    root.visit_prefix(prefix, visitor);
  }

#ifdef PRINT_RADIX_TREE
  template <typename Char, typename Traits>
  friend std::basic_ostream<Char, Traits> &
  operator<<(typename std::basic_ostream<Char, Traits>& out,
             const RadixTree<T> &rt) {
    return out << rt.root;
  }
#endif /* PRINT_RADIX_TREE */
};

#endif
