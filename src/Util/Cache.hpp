/*
 * Copyright (C) 2011 Max Kellermann <max@duempel.org>
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

#ifndef CACHE_HPP
#define CACHE_HPP

#include "ListHead.hpp"
#include "Compiler.h"

#include <unordered_map>
#include <limits>
#include <assert.h>

template<typename Key, typename Data,
         unsigned capacity,
         typename Hash=std::hash<Key>,
         typename KeyEqual=std::equal_to<Key>>
class Cache {

  /**
   * Wrapper that holds an uninitialised object, which be be
   * initialised and deinitialised on demand.
   */
  template<typename T>
  class Constructible {
    char buffer[sizeof(T)];

#ifndef NDEBUG
    bool constructed;
#endif

  public:
#ifndef NDEBUG
    Constructible():constructed(false) {}

    ~Constructible() {
      assert(!constructed);
    }
#endif

    const T &Get() const {
      assert(constructed);

      const void *p = static_cast<const void *>(buffer);
      return *static_cast<const T *>(p);
    }

    T &Get() {
      assert(constructed);

      void *p = static_cast<void *>(buffer);
      return *static_cast<T *>(p);
    }

    void Construct() {
      assert(!constructed);

      void *p = static_cast<void *>(buffer);
      new (p) T();

#ifndef NDEBUG
      constructed = true;
#endif
    }

    template<typename U>
    void Construct(U &&value) {
      assert(!constructed);

      void *p = static_cast<void *>(buffer);
      new (p) T(std::forward<U>(value));

#ifndef NDEBUG
      constructed = true;
#endif
    }

    void Destruct() {
      assert(constructed);

      T &value = Get();
      value.T::~T();

#ifndef NDEBUG
      constructed = false;
#endif
    }
  };

  class Item;

  /* This is a multimap, even though we use at most one cache item for
     a key.  A multimap means less overhead, because insert() does not
     need to do a full lookup, and this class  */
  typedef std::unordered_multimap<Key, class Item *, Hash, KeyEqual> KeyMap;

  class Item : public ListHead {
    /**
     * This iterator is stored to allow quick removal of outdated
     * cache items.  This is safe, because rehashing is disabled in
     * #KeyMap.
     */
    typename KeyMap::iterator iterator;

    Constructible<Data> data;

  public:
    typename KeyMap::iterator GetIterator() {
      return iterator;
    }

    void SetIterator(typename KeyMap::iterator _iterator) {
      iterator = _iterator;
    }

    const Data &GetData() const {
      return data.Get();
    }

    template<typename U>
    void Construct(U &&value) {
      data.Construct(std::forward<U>(value));
    }

    void Destruct() {
      data.Destruct();
    }

    template<typename U>
    void Replace(U &&value) {
      data.Get() = std::forward<U>(value);
    }
  };

  /**
   * The number of cached items.
   */
  unsigned size;

  /**
   * The list of unallocated items.
   */
  ListHead unallocated_list;

  ListHead chronological_list;

  KeyMap map;

  Item buffer[capacity];

  Item &GetOldest() {
    assert(!chronological_list.IsEmpty());

    return *(Item *)chronological_list.GetPrevious();
  }

  /**
   * Remove the oldest item from the cache (both from the #map and
   * from #chronological_list), but do not destruct it.
   */
  Item &RemoveOldest() {
    Item &item = GetOldest();

    map.erase(item.GetIterator());
    item.Remove();

#ifndef NDEBUG
    --size;
#endif

    return item;
  }

  /**
   * Allocate an item from #unallocated_list, but do not construct it.
   */
  Item &Allocate() {
    assert(!unallocated_list.IsEmpty());

    Item &item = *(Item *)unallocated_list.GetNext();
    item.Remove();
    return item;
  }

  template<typename U>
  Item &Make(U &&data) {
    if (unallocated_list.IsEmpty()) {
      /* cache is full: delete oldest */
      Item &item = RemoveOldest();
      item.Replace(std::forward<U>(data));
      return item;
    } else {
      /* cache is not full: allocate new item */
      Item &item = Allocate();
      item.Construct(std::forward<U>(data));
      return item;
    }
  }

public:
  Cache()
    :size(0),
     unallocated_list(ListHead::empty()),
     chronological_list(ListHead::empty()) {
    for (unsigned i = 0; i < capacity; ++i)
      buffer[i].InsertAfter(unallocated_list);

    /* allocate enough buckets for the whole lifetime of this
       object */
    map.rehash(capacity);

    /* forcibly disable rehashing, as that would Invalidate existing
       iterators */
    map.max_load_factor(std::numeric_limits<decltype(map.max_load_factor())>::infinity());
  }

  ~Cache() {
    Clear();
  }

  bool IsFull() const {
    assert(size <= capacity);

    return size == capacity;
  }

  void Clear() {
    map.clear();

    while (!chronological_list.IsEmpty()) {
#ifndef NDEBUG
      assert(size > 0);
      --size;
#endif

      Item &item = *(Item *)chronological_list.GetNext();
      item.Remove();

      item.Destruct();
      item.InsertAfter(unallocated_list);
    }

    assert(size == 0);
    size = 0;
  }

  const Data *Get(const Key &key) {
    typename KeyMap::iterator i = map.find(key);
    if (i == map.end())
      return nullptr;

    Item &item = *i->second;
    assert(item.GetIterator() == i);

    /* move to the front of the chronological list */
    item.Remove();
    item.InsertAfter(chronological_list);

    return &item.GetData();
  }

  template<typename K, typename U>
  void Put(K &&key, U &&data) {
    assert(map.find(key) == map.end());

    Item &item = Make(std::forward<U>(data));
    item.InsertAfter(chronological_list);
    auto iterator = map.insert(std::make_pair(std::forward<K>(key), &item));
    item.SetIterator(iterator);

#ifndef NDEBUG
    ++size;
#endif
  }
};

#endif
