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

#ifndef XCSOAR_UTIL_LIST_HEAD_HPP
#define XCSOAR_UTIL_LIST_HEAD_HPP

#include "Compiler.h"

#include <cassert>

/**
 * A doubly linked list implementation, similar to linux/list.h.  It
 * is very cheap at runtime, because the prev/next pointers are
 * included in each item, instead of doing a separate allocation.  To
 * use it, derive your class/struct from this one.
 */
class ListHead {
  ListHead *prev, *next;

public:
  /**
   * Cheap non-initializing constructor.
   */
  ListHead() {}

  struct empty {};

  /**
   * Initialize an empty list.
   */
  ListHead(empty):prev(this), next(this) {}

  const ListHead *GetPrevious() const {
    return prev;
  }

  ListHead *GetPrevious() {
    return prev;
  }

  const ListHead *GetNext() const {
    return next;
  }

  ListHead *GetNext() {
    return next;
  }

  bool IsEmpty() const {
    assert((prev == this) == (next == this));
    return prev == this;
  }

  /**
   * Is the specified item the first one in the list?
   */
  bool IsFirst(const ListHead &item) const {
    return item.prev == this;
  }

  /**
   * Is the specified item the last one in the list?
   */
  bool IsLast(const ListHead &item) const {
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
    unsigned n = 0;
    for (const ListHead *p = next; p != this; p = p->next)
      ++n;
    return n;
  }

  /**
   * Turns this object into an empty list.
   */
  void Clear() {
    prev = next = this;
  }

  /**
   * Remove this item from the linked list.
   */
  void Remove() {
    assert(!IsEmpty());
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
    assert(&other != this);
    assert(&other != this);
    assert(other.prev->next == &other);
    assert(other.next->prev == &other);

    next = other.next;
    next->prev = this;
    prev = &other;
    other.next = this;
  }

  /**
   * Insert this item before the specified one.  It must not be in the
   * list already (or in another list).
   */
  void InsertBefore(ListHead &other) {
    assert(&other != this);
    assert(&other != this);
    assert(other.prev->next == &other);
    assert(other.next->prev == &other);

    prev = other.prev;
    prev->next = this;
    next = &other;
    other.prev = this;
  }

  /**
   * Move this item from its current position in the list after the
   * specified one.
   */
  void MoveAfter(ListHead &other) {
    if (prev == &other) {
      assert(other.next == this);
      return;
    }

    Remove();
    InsertAfter(other);
  }
};

#endif
