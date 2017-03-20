/*
 * Copyright (C) 2010 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include "Util/StaticString.hxx"
#include "StringCompare.hxx"
#include "tstring.hpp"

#include <algorithm>
#include <assert.h>
#include <tchar.h>

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

    constexpr
    LeafList():head(nullptr) {}

    ~LeafList() {
      Leaf *next = head;
      while (next != nullptr) {
        Leaf *leaf = next;
        next = leaf->next;

        delete leaf;
      }
    }

    void Clear() {
      Leaf *next = head;
      while (next != nullptr) {
        Leaf *leaf = next;
        next = leaf->next;

        delete leaf;
      }

      head = nullptr;
    }

    void Swap(LeafList &other) {
      std::swap(head, other.head);
    }

    void Add(const T &value) {
      head = new Leaf(head, value);
    }

    bool Remove(const T &value) {
      Leaf **leaf_r = &head;

      while (*leaf_r != nullptr) {
        Leaf *leaf = *leaf_r;
        if (leaf->value == value) {
          *leaf_r = leaf->next;
          leaf->next = nullptr;
          delete leaf;
          return true;
        }

        leaf_r = &leaf->next;
      }

      return false;
    }

    T *GetFirstPointer() {
      return head != nullptr
        ? &head->value
        : nullptr;
    }

    const T *GetFirstPointer() const {
      return head != nullptr
        ? &head->value
        : nullptr;
    }

    template<class P>
    T *GetIf(const P &predicate) {
        for (Leaf *leaf = head; leaf != nullptr; leaf = leaf->next)
          if (predicate(leaf->value))
            return &leaf->value;

        return nullptr;
    }

    template<class P>
    const T *GetIf(const P &predicate) const {
        for (Leaf *leaf = head; leaf != nullptr; leaf = leaf->next)
          if (predicate(leaf->value))
            return &leaf->value;

        return nullptr;
    }

    template<typename V>
    void VisitAll(V &visitor) {
      for (Leaf *leaf = head; leaf != nullptr; leaf = leaf->next)
        visitor(leaf->value);
    }

    template<typename V>
    void VisitAll(V &visitor) const {
      for (const Leaf *leaf = head; leaf != nullptr; leaf = leaf->next)
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
  struct Node {
    StaticString<8> label;
    Node *next_sibling, *children;
    LeafList leaves;

    constexpr
    Node(const TCHAR *_label)
      :label(_label),
       next_sibling(nullptr), children(nullptr) {}

    Node(const Node &) = delete;

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
      while (_tcslen(label) >= Node::label.CAPACITY) {
        /* label too long for the Node's StaticString, create another
           child Node */
        label += Node::label.CAPACITY - 1;
        Node *node = new Node(label);
        bottom->children = node;
        bottom = node;
      }

      bottom->AddValue(value);
      return top;
    }

    void Clear() {
      delete children;
      children = nullptr;
      leaves.Clear();
    }

    /**
     * Find the first mismatching character.  Returns the original
     * parameter if there is no match at all.  Returns the end of the
     * string if there is a full match (even if the node's label is
     * longer).
     */
    const TCHAR *MatchKey(const TCHAR *key) const {
      const TCHAR *l = label.c_str();

      while (!StringIsEmpty(key) && *key == *l) {
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
     * is outside of the label's scope.  Returns nullptr if there is a
     * mismatch.
     */
    const TCHAR *MatchPrefix(const TCHAR *prefix) const {
      const TCHAR *l = label.c_str();

      while (!StringIsEmpty(prefix) && !StringIsEmpty(l)) {
        if (*l != *prefix)
          return nullptr;

        ++prefix;
        ++l;
      }

      return prefix;
    }

    T *Get(const TCHAR *key) {
      if (StringIsEmpty(key))
        /* found */
        return leaves.GetFirstPointer();

      const auto m = FindChild(key);
      return m.IsFullMatch(key)
        ? m.node->Get(m.key)
        : nullptr;
    }

    const T *Get(const TCHAR *key) const {
      if (StringIsEmpty(key))
        /* found */
        return leaves.GetFirstPointer();

      const auto m = FindChild(key);
      return m.IsFullMatch(key)
        ? m.node->Get(m.key)
        : nullptr;
    }

    template<class P>
    T *GetIf(const TCHAR *key, const P &predicate) {
      if (StringIsEmpty(key))
        /* found */
        return leaves.GetIf(predicate);

      const auto m = FindChild(key);
      return m.IsFullMatch(key)
        ? m.node->GetIf(m.key, predicate)
        : nullptr;
    }

    template<class P>
    const T *GetIf(const TCHAR *key, const P &predicate) const {
      if (StringIsEmpty(key))
        /* found */
        return leaves.GetIf(predicate);

      const auto m = FindChild(key);
      return m.IsFullMatch(key)
        ? m.node->GetIf(m.key, predicate)
        : nullptr;
    }

    TCHAR *Suggest(const TCHAR *prefix, TCHAR *dest, size_t max_length) const {
      if (StringIsEmpty(prefix)) {
        /* exact match - return the first character of all child
           nodes */
        TCHAR *retval = dest, *end = dest + max_length - 1;

        for (const Node *node = children; node != nullptr && dest < end;
             node = node->next_sibling)
          *dest++ = node->label[0u];

        *dest = _T('\0');
        return retval;
      }

      const auto m = FindChild(prefix);
      if (m.key == prefix)
        /* mismatch */
        return nullptr;

      if (m.IsFullMatch(prefix))
        /* recurse */
        return m.node->Suggest(m.key, dest, max_length);

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
    void Split(size_t length) {
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
    void AddValue(const T &value) {
      leaves.Add(value);
    }

    /**
     * Remove all values of this node.
     */
    void RemoveValues() {
      leaves.Clear();
    }

    /**
     * Remove the specified value.  If there are multiple instances of
     * the same value, only one is removed.
     *
     * @return true if a value was found and removed
     */
    bool RemoveValue(const T &value) {
      return leaves.Remove(value);
    }

    /**
     * Remove all values with the specified key.
     */
    void RemoveValues(const TCHAR *key) {
      assert(key != nullptr);

      if (StringIsEmpty(key)) {
        /* this is the right node */
        RemoveValues();
      } else {
        const auto m = FindChild(key);
        if (m.IsFullMatch(key))
          /* recurse */
          m.node->RemoveValues(m.key);
      }
    }

    /**
     * Remove the specified value.
     *
     * @return true if a value was found and removed
     */
    bool RemoveValue(const TCHAR *key, const T &value) {
      assert(key != nullptr);

      if (StringIsEmpty(key)) {
        /* this is the right node */
        return RemoveValue(value);
      } else {
        const auto m = FindChild(key);
        return m.IsFullMatch(key) &&
          m.node->RemoveValue(m.key, value);
      }
    }

    /**
     * Visit all direct values of this node in no specific order.
     */
    template<typename V>
    void VisitValues(V &visitor) {
      leaves.VisitAll(visitor);
    }

    /**
     * Visit all direct values of this node in no specific order.
     */
    template<typename V>
    void VisitValues(V &visitor) const {
      leaves.VisitAll(visitor);
    }

    /**
     * Visit all direct values of this node in no specific order.
     * This overload is used by VisitAllPairs().
     */
    template<typename V>
    void VisitValues(const TCHAR *prefix, V &visitor) const {
      tstring key(prefix);
      key.append(label);

      const KeyVisitorAdapter<V> adapter(visitor, key.c_str());
      VisitValues(adapter);
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.
     */
    template<typename V>
    void VisitAllChildren(V &visitor) {
      for (Node *node = children; node != nullptr; node = node->next_sibling) {
        node->VisitValues(visitor);
        node->VisitAllChildren(visitor);
      }
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.
     */
    template<typename V>
    void VisitAllChildren(V &visitor) const {
      for (const Node *node = children; node != nullptr;
           node = node->next_sibling) {
        node->VisitValues(visitor);
        node->VisitAllChildren(visitor);
      }
    }

    /**
     * Recursively visit all child nodes and their values in
     * alphabetic order.  This overload is used by VisitAllPairs().
     */
    template<typename V>
    void VisitAllChildren(const TCHAR *prefix, V &visitor) const {
      tstring key(prefix);
      key.append(label);

      for (const Node *node = children; node != nullptr;
           node = node->next_sibling) {
        node->VisitValues(key.c_str(), visitor);
        node->VisitAllChildren(key.c_str(), visitor);
      }
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void VisitChildren(const TCHAR *key, V &visitor) {
      VisitSiblings(children, key, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void VisitChildren(const TCHAR *key, V &visitor) const {
      VisitSiblings(const_cast<const Node *>(children), key, visitor);
    }

    /**
     * Recursively visit all values with the specified key.  The
     * key is matched on this node's label first.
     */
    template<typename V>
    void Visit(const TCHAR *key, V &visitor) {
      const TCHAR *match = MatchPrefix(key);
      if (match == nullptr)
        return;

      if (StringIsEmpty(match))
        VisitValues(visitor);
      else
        VisitChildren(match, visitor);
    }

    /**
     * Recursively visit all values with the specified key.  The
     * key is matched on this node's label first.
     */
    template<typename V>
    void Visit(const TCHAR *key, V &visitor) const {
      const TCHAR *match = MatchPrefix(key);
      if (match == nullptr)
        return;

      if (StringIsEmpty(match))
        VisitValues(visitor);
      else
        VisitChildren(match, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void VisitPrefixChildren(const TCHAR *prefix, V &visitor) {
      if (StringIsEmpty(prefix))
        VisitAllChildren(visitor);
      else
        for (Node *node = children; node != nullptr; node = node->next_sibling)
          node->VisitPrefix(prefix, visitor);
    }

    /**
     * Recursively visit all child nodes with the specified prefix in
     * alphabetic order.  The prefix is only matched on children's
     * labels.
     */
    template<typename V>
    void VisitPrefixChildren(const TCHAR *prefix, V &visitor) const {
      if (StringIsEmpty(prefix))
        VisitAllChildren(visitor);
      else
        for (const Node *node = children; node != nullptr;
             node = node->next_sibling)
          node->VisitPrefix(prefix, visitor);
    }

    /**
     * Recursively visit all values and child nodes with the specified
     * prefix in alphabetic order.  The prefix is matched on this
     * node's label first.
     */
    template<typename V>
    void VisitPrefix(const TCHAR *prefix, V &visitor) {
      const TCHAR *match = MatchPrefix(prefix);
      if (match == nullptr)
        return;

      if (StringIsEmpty(match)) {
        VisitValues(visitor);
        VisitAllChildren(visitor);
      } else {
        VisitPrefixChildren(match, visitor);
      }
    }

    /**
     * Recursively visit all values and child nodes with the specified
     * prefix in alphabetic order.  The prefix is matched on this
     * node's label first.
     */
    template<typename V>
    void VisitPrefix(const TCHAR *prefix, V &visitor) const {
      const TCHAR *match = MatchPrefix(prefix);
      if (match == nullptr)
        return;

      if (StringIsEmpty(match)) {
        VisitValues(visitor);
        VisitAllChildren(visitor);
      } else {
        VisitPrefixChildren(match, visitor);
      }
    }

    struct Match {
      Node *node;
      const TCHAR *key;

      Match(Node *_node, const TCHAR *_key)
        :node(_node), key(_key) {}

      bool IsFullMatch(const TCHAR *key) const {
        return this->key != key && this->key >= key + node->label.length();
      }
    };

    /**
     * Find a matching child node.  Returns a pair containing the node
     * pointer (or nullptr if this node has no children), and a pointer
     * to the portion of the key which was not used yet.
     */
    struct Match FindChild(const TCHAR *key) const {
      Node *node = children, *prev = nullptr;
      while (node != nullptr) {
        const TCHAR *label = node->label.c_str();
        if (key[0u] < label[0u])
          return Match(prev, key);
        else if (key[0u] == label[0u])
          return Match(node, node->MatchKey(key));

        prev = node;
        node = node->next_sibling;
      }

      return Match(prev, key);
    }

    /**
     * Adds a new value relative to this node, possibly creating a new
     * node and/or splitting an existing node.
     */
    void Add(const TCHAR *key, const T &value) {
      assert(key != nullptr);

      if (StringIsEmpty(key)) {
        /* add to this node */
        AddValue(value);
        return;
      }

      const auto m = FindChild(key);
      if (m.key == key) {
        /* no match - create new node */
        Node *node = CreateLeaf(key, value);

        if (m.node == nullptr) {
          /* insert before list head */
          node->next_sibling = children;
          children = node;
        } else {
          /* insert after that node */
          node->next_sibling = m.node->next_sibling;
          m.node->next_sibling = node;
        }
      } else if (m.IsFullMatch(key)) {
        m.node->Add(m.key, value);
      } else {
        /* split existing node */
        m.node->Split(m.key - key);

        if (StringIsEmpty(m.key)) {
          /* add to splitted parent node */
          m.node->AddValue(value);
        } else {
          Node *node = CreateLeaf(m.key, value);

          if (m.key[0u] < m.node->children->label[0u]) {
            /* insert before list head */
            node->next_sibling = m.node->children;
            m.node->children = node;
          } else {
            /* insert after the splitted child node */
            assert(m.node->children->next_sibling == nullptr);

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
      for (const RadixTree<T>::Leaf *leaf = node.leaves; leaf != nullptr;
           leaf = leaf->next)
        out << "  value " << leaf->value << "\n";
      for (const RadixTree<T>::Node *child = node.children; child != nullptr;
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
  constexpr
  RadixTree():root(_T("")) {}

  /**
   * Gets a value for the specified key.  Returns the parameter
   * default_value if the specified key is not present.  If there are
   * multiple values, any one is returned.
   */
  T &Get(const TCHAR *key, T &default_value) {
    T *value = root.get(key);
    return value != nullptr
      ? *value
      : default_value;
  }

  /**
   * Gets a value for the specified key.  Returns the parameter
   * default_value if the specified key is not present.  If there are
   * multiple values, any one is returned.
   */
  const T &Get(const TCHAR *key, const T &default_value) const {
    const T *value = root.Get(key);
    return value != nullptr
      ? *value
      : default_value;
  }

  template<class P>
  T &GetIf(const TCHAR *key, T &default_value, const P &predicate) {
    const T *value = root.GetIf(key, predicate);
    return value != nullptr
      ? *value
      : default_value;
  }

  template<class P>
  const T &GetIf(const TCHAR *key, const T &default_value,
                 const P &predicate) const {
    const T *value = root.GetIf(key, predicate);
    return value != nullptr
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
   * @return the destination buffer, or nullptr if the prefix does not
   * occur in the tree
   */
  TCHAR *Suggest(const TCHAR *prefix, TCHAR *dest, size_t max_length) const {
    return root.Suggest(prefix, dest, max_length);
  }

  void Clear() {
    root.Clear();
  }

  /**
   * Add a new value with the specified key.  Multiple values can
   * exist for one key.
   */
  void Add(const TCHAR *key, const T &value) {
    root.Add(key, value);
  }

  /**
   * Remove all values with the specified key.
   */
  void Remove(const TCHAR *key) {
    assert(key != nullptr);

    root.RemoveValues(key);
  }

  /**
   * Remove a value with the specified key.
     *
     * @return true if a value was found and removed
   */
  bool Remove(const TCHAR *key, const T &value) {
    assert(key != nullptr);

    return root.RemoveValue(key, value);
  }

  /**
   * Visit all values in alphabetic order.
   */
  template<typename V>
  void VisitAll(V &visitor) {
    root.VisitValues(visitor);
    root.VisitAllChildren(visitor);
  }

  /**
   * Visit all values in alphabetic order.
   */
  template<typename V>
  void VisitAll(V &visitor) const {
    root.VisitValues(visitor);
    root.VisitAllChildren(visitor);
  }

  /**
   * Visit all key/value pairs in alphabetic order.
   */
  template<typename V>
  void VisitAllPairs(V &visitor) const {
    root.VisitValues(_T(""), visitor);
    root.VisitAllChildren(_T(""), visitor);
  }

  /**
   * Visit all values with the specified key.
   */
  template<typename V>
  void Visit(const TCHAR *key, V &visitor) {
    root.Visit(key, visitor);
  }

  /**
   * Visit all values with the specified key.
   */
  template<typename V>
  void Visit(const TCHAR *key, V &visitor) const {
    root.Visit(key, visitor);
  }

  /**
   * Visit all values matching the specified prefix in alphabetic
   * order.
   */
  template<typename V>
  void VisitPrefix(const TCHAR *prefix, V &visitor) {
    root.VisitPrefix(prefix, visitor);
  }

  /**
   * Visit all values matching the specified prefix in alphabetic
   * order.
   */
  template<typename V>
  void VisitPrefix(const TCHAR *prefix, V &visitor) const {
    root.VisitPrefix(prefix, visitor);
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
