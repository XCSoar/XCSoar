/*
 * Copyright (C) 2010-2011 Max Kellermann <max@duempel.org>
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

#ifndef LIST_HEAD_HPP
#define LIST_HEAD_HPP

#include "Compiler.h"

#include <stddef.h>
#include <iterator>
#include <cassert>

/**
 * A doubly linked list implementation, similar to linux/list.h.  It
 * is very cheap at runtime, because the prev/next pointers are
 * included in each item, instead of doing a separate allocation.  To
 * use it, derive your class/struct from this one.
 */
class ListHead {
  ListHead *next, *prev;

#ifndef NDEBUG
  enum {
    UNKNOWN,
    HEAD,
    CONNECTED,
    DISCONNECTED,
  } type;
#endif

public:
  /**
   * Cheap non-initializing constructor.
   */
#ifndef NDEBUG
  ListHead()
    :type(UNKNOWN) {}
#else
  ListHead() = default;
#endif

  struct empty {};

  /**
   * Initialize an empty list.
   */
  ListHead(empty)
    :next(this), prev(this)
#ifndef NDEBUG
    , type(HEAD)
#endif
  {}

  const ListHead *GetPrevious() const {
    assert(type == CONNECTED || type == HEAD);

    return prev;
  }

  ListHead *GetPrevious() {
    assert(type == CONNECTED || type == HEAD);

    return prev;
  }

  const ListHead *GetNext() const {
    assert(type == CONNECTED || type == HEAD);

    return next;
  }

  ListHead *GetNext() {
    assert(type == CONNECTED || type == HEAD);

    return next;
  }

  bool IsEmpty() const {
    assert(type == HEAD);
    assert((prev == this) == (next == this));
    return prev == this;
  }

  /**
   * Is the specified item the first one in the list?
   */
  bool IsFirst(const ListHead &item) const {
    assert(type == HEAD);

    return item.prev == this;
  }

  /**
   * Is the specified item the last one in the list?
   */
  bool IsLast(const ListHead &item) const {
    assert(type == HEAD);

    return item.next == this;
  }

  /**
   * Is the specified item the first or the last one in the list?
   */
  bool IsEdge(const ListHead &item) const {
    return IsFirst(item) || IsLast(item);
  }

  /**
   * Count the number of items in this list, not including the current
   * one.
   */
  gcc_pure
  unsigned Count() const {
    assert(type == HEAD);

    unsigned n = 0;
    for (const ListHead *p = next; p != this; p = p->next)
      ++n;
    return n;
  }

  /**
   * Turns this object into an empty list.
   */
  void Clear() {
#ifndef NDEBUG
    assert(type == UNKNOWN || type == HEAD);
#endif

    prev = next = this;

#ifndef NDEBUG
    type = HEAD;
#endif
  }

  /**
   * Remove this item from the linked list.
   */
  void Remove() {
    assert(type == CONNECTED);
    assert(prev->next == this);
    assert(next->prev == this);

    next->prev = prev;
    prev->next = next;

#ifndef NDEBUG
    type = DISCONNECTED;
#endif
  }

  /**
   * Remove this item from the linked list.  This variant allows
   * removing a const object from the list.  Debug information is not
   * updated, this object must not ever be used again.  Use with care!
   */
  void RemoveConst() const {
    assert(type == CONNECTED);
    assert(prev->next == this);
    assert(next->prev == this);

    next->prev = prev;
    prev->next = next;
  }

  /**
   * Insert this item after the specified one.  It must not be in the
   * list already (or in another list).
   */
  void InsertAfter(ListHead &other) {
    assert(type == UNKNOWN || type == DISCONNECTED);
    assert(&other != this);
    assert(&other != this);
    assert(other.type == HEAD || other.type == CONNECTED);
    assert(other.prev->next == &other);
    assert(other.next->prev == &other);

    next = other.next;
    next->prev = this;
    prev = &other;
    other.next = this;

#ifndef NDEBUG
    type = CONNECTED;
#endif
  }

  /**
   * Insert this item before the specified one.  It must not be in the
   * list already (or in another list).
   */
  void InsertBefore(ListHead &other) {
    assert(type == UNKNOWN || type == DISCONNECTED);
    assert(&other != this);
    assert(&other != this);
    assert(other.type == HEAD || other.type == CONNECTED);
    assert(other.prev->next == &other);
    assert(other.next->prev == &other);

    prev = other.prev;
    prev->next = this;
    next = &other;
    other.prev = this;

#ifndef NDEBUG
    type = CONNECTED;
#endif
  }

  /**
   * Move this item from its current position in the list after the
   * specified one.
   */
  void MoveAfter(ListHead &other) {
    assert(type == CONNECTED);
    assert(other.type == HEAD || other.type == CONNECTED);

    if (prev == &other) {
      assert(other.next == this);
      return;
    }

    Remove();
    InsertAfter(other);
  }

  /**
   * Insert this item with the specified one.
   */
  void Replace(ListHead &other) {
    assert(type == CONNECTED);
    assert(&other != this);
    assert(prev->next == this);
    assert(next->prev == this);
    assert(other.type == UNKNOWN || other.type == DISCONNECTED);

    other.next = next;
    other.prev = prev;

    next->prev = &other;
    prev->next = &other;

#ifndef NDEBUG
    type = DISCONNECTED;
    other.type = CONNECTED;
#endif
  }

  /**
   * Changes the type from CONNECTED to DISCONNECTED.  Call this on
   * the copy of an existing object.
   */
  void SetDisconnected() {
#ifndef NDEBUG
    assert(type == CONNECTED);
    type = DISCONNECTED;
#endif
  }

  class const_iterator {
    friend class ListHead;

    const ListHead *current;

    constexpr
    const_iterator(const ListHead *_current):current(_current) {}

  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef const ListHead value_type;
    typedef const ListHead *pointer;
    typedef const ListHead &reference;

    const_iterator() = default;

    reference operator*() const {
      return *current;
    }

    pointer operator->() const {
      return current;
    }

    const_iterator &operator++() {
      current = current->next;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator old = *this;
      current = current->next;
      return old;
    }

    const_iterator &operator--() {
      current = current->prev;
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator old = *this;
      current = current->prev;
      return old;
    }

    bool operator==(const const_iterator &other) const {
      return current == other.current;
    }

    bool operator!=(const const_iterator &other) const {
      return current != other.current;
    }
  };

  const_iterator begin() const {
    assert(type == HEAD);

    return next;
  }

  const_iterator end() const {
    assert(type == HEAD);

    return this;
  }

  class const_reverse_iterator {
    friend class ListHead;

    const ListHead *current;

    constexpr
    const_reverse_iterator(const ListHead *_current):current(_current) {}

  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef const ListHead value_type;
    typedef const ListHead *pointer;
    typedef const ListHead &reference;

    const_reverse_iterator() = default;

    reference operator*() const {
      return *current;
    }

    pointer operator->() const {
      return current;
    }

    const_reverse_iterator &operator++() {
      current = current->prev;
      return *this;
    }

    const_reverse_iterator operator++(int) {
      const_reverse_iterator old = *this;
      current = current->prev;
      return old;
    }

    const_reverse_iterator &operator--() {
      current = current->next;
      return *this;
    }

    const_reverse_iterator operator--(int) {
      const_reverse_iterator old = *this;
      current = current->next;
      return old;
    }

    bool operator==(const const_reverse_iterator &other) const {
      return current == other.current;
    }

    bool operator!=(const const_reverse_iterator &other) const {
      return current != other.current;
    }
  };

  const_reverse_iterator rbegin() const {
    assert(type == HEAD);

    return prev;
  }

  const_reverse_iterator rend() const {
    assert(type == HEAD);

    return this;
  }
};

#endif
