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

#include "Manual.hpp"
#include "Compiler.h"

#include <boost/intrusive/list.hpp>

#include <unordered_map>
#include <limits>
#include <assert.h>

template<typename Key, typename Data,
         unsigned capacity,
         typename Hash=std::hash<Key>,
         typename KeyEqual=std::equal_to<Key>>
class Cache {

  class Item;

  /* This is a multimap, even though we use at most one cache item for
     a key.  A multimap means less overhead, because insert() does not
     need to do a full lookup, and this class  */
  typedef std::unordered_multimap<Key, class Item *, Hash, KeyEqual> KeyMap;

  class Item : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> {
    /**
     * This iterator is stored to allow quick removal of outdated
     * cache items.  This is safe, because rehashing is disabled in
     * #KeyMap.
     */
    typename KeyMap::iterator iterator;

    Manual<Data> data;

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

  typedef boost::intrusive::list<Item,
                                 boost::intrusive::constant_time_size<false>> ItemList;

  /**
   * The number of cached items.
   */
  unsigned size;

  /**
   * The list of unallocated items.
   */
  ItemList unallocated_list;

  ItemList chronological_list;

  KeyMap map;

  Item buffer[capacity];

  Item &GetOldest() {
    assert(!chronological_list.empty());

    return chronological_list.back();
  }

  /**
   * Remove the oldest item from the cache (both from the #map and
   * from #chronological_list), but do not destruct it.
   */
  Item &RemoveOldest() {
    Item &item = GetOldest();

    map.erase(item.GetIterator());
    chronological_list.erase(chronological_list.iterator_to(item));

#ifndef NDEBUG
    --size;
#endif

    return item;
  }

  /**
   * Allocate an item from #unallocated_list, but do not construct it.
   */
  Item &Allocate() {
    assert(!unallocated_list.empty());

    Item &item = unallocated_list.front();
    unallocated_list.erase(unallocated_list.iterator_to(item));
    return item;
  }

  template<typename U>
  Item &Make(U &&data) {
    if (unallocated_list.empty()) {
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
    :size(0) {
    for (unsigned i = 0; i < capacity; ++i)
      unallocated_list.push_back(buffer[i]);

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

    chronological_list.clear_and_dispose([this](Item *item){
#ifndef NDEBUG
        assert(size > 0);
        --size;
#endif

        item->Destruct();
        unallocated_list.push_front(*item);
      });

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
    chronological_list.erase(chronological_list.iterator_to(item));
    chronological_list.push_front(item);

    return &item.GetData();
  }

  template<typename K, typename U>
  void Put(K &&key, U &&data) {
    assert(map.find(key) == map.end());

    Item &item = Make(std::forward<U>(data));
    chronological_list.push_front(item);
    auto iterator = map.insert(std::make_pair(std::forward<K>(key), &item));
    item.SetIterator(iterator);

#ifndef NDEBUG
    ++size;
#endif
  }
};

#endif
