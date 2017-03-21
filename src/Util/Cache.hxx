/*
 * Copyright (C) 2011-2017 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef CACHE_HXX
#define CACHE_HXX

#include "Manual.hxx"
#include "Compiler.h"

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>

#include <array>

#include <assert.h>

/**
 * A simple LRU cache.  Item lookup is done with a hash table.  No
 * dynamic allocation; all items are allocated statically inside this
 * class.
 *
 * @param max_size the maximum number of items in the cache
 * @param table_size the size of the internal hash table; rule of
 * thumb: should be prime
 */
template<typename Key, typename Data,
	 std::size_t max_size,
	 std::size_t table_size,
	 typename Hash=std::hash<Key>,
	 typename Equal=std::equal_to<Key>>
class Cache {

	struct Pair {
		Key key;
		Data data;

		template<typename K, typename U>
		Pair(K &&_key, U &&_data)
			:key(std::forward<K>(_key)),
			 data(std::forward<U>(_data)) {}

		template<typename K, typename U>
		void Replace(K &&_key, U &&_data) {
			key = std::forward<K>(_key);
			data = std::forward<U>(_data);
		}
	};

	class Item
		: public boost::intrusive::unordered_set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>,
		  public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> {

		Manual<Pair> pair;

	public:
		const Key &GetKey() const {
			return pair->key;
		}

		const Data &GetData() const {
			return pair->data;
		}

		template<typename K, typename U>
		void Construct(K &&_key, U &&value) {
			pair.Construct(std::forward<K>(_key),
				       std::forward<U>(value));
		}

		void Destruct() {
			pair.Destruct();
		}

		template<typename K, typename U>
		void Replace(K &&_key, U &&value) {
			pair->Replace(std::forward<K>(_key),
				      std::forward<U>(value));
		}
	};

	struct ItemHash : Hash {
		using Hash::operator();

		gcc_pure
		std::size_t operator()(const Item &a) const {
			return Hash::operator()(a.GetKey());
		}
	};

	struct ItemEqual : Equal {
		gcc_pure
		bool operator()(const Item &a, const Item &b) const {
			return Equal::operator()(a.GetKey(), b.GetKey());
		}

		gcc_pure
		bool operator()(const Key &a, const Item &b) const {
			return Equal::operator()(a, b.GetKey());
		}
	};

	typedef boost::intrusive::list<Item,
				       boost::intrusive::constant_time_size<false>> ItemList;

	/**
	 * The list of unallocated items.
	 */
	ItemList unallocated_list;

	ItemList chronological_list;

	/* This is a multiset, even though we use at most one cache item for
	   a key.  A multiset means less overhead, because insert() does not
	   need to do a full lookup, and this class will only insert new
	   items when it knows an item does not already exists */
	typedef boost::intrusive::unordered_multiset<Item,
						     boost::intrusive::hash<ItemHash>,
						     boost::intrusive::equal<ItemEqual>,
						     boost::intrusive::constant_time_size<false>> KeyMap;

	std::array<typename KeyMap::bucket_type, table_size> buckets;

	KeyMap map;

	std::array<Item, max_size> buffer;

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

		map.erase(map.iterator_to(item));
		chronological_list.erase(chronological_list.iterator_to(item));

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

	template<typename K, typename U>
	Item &Make(K &&key, U &&data) {
		if (unallocated_list.empty()) {
			/* cache is full: delete oldest */
			Item &item = RemoveOldest();
			item.Replace(std::forward<K>(key), std::forward<U>(data));
			return item;
		} else {
			/* cache is not full: allocate new item */
			Item &item = Allocate();
			item.Construct(std::forward<K>(key), std::forward<U>(data));
			return item;
		}
	}

public:
	Cache()
		:map(typename KeyMap::bucket_traits(&buckets.front(), buckets.size())) {
		for (auto &i : buffer)
			unallocated_list.push_back(i);
	}

	~Cache() {
		Clear();
	}

	Cache(const Cache &) = delete;
	Cache &operator=(const Cache &) = delete;

	bool IsFull() const {
		return unallocated_list.empty();
	}

	void Clear() {
		map.clear();

		chronological_list.clear_and_dispose([this](Item *item){
				item->Destruct();
				unallocated_list.push_front(*item);
			});
	}

	/**
	 * Look up an item by its key.  Returns nullptr if no such
	 * item exists.
	 */
	const Data *Get(const Key &key) {
		auto i = map.find(key, map.hash_function(), map.key_eq());
		if (i == map.end())
			return nullptr;

		Item &item = *i;

		/* move to the front of the chronological list */
		chronological_list.erase(chronological_list.iterator_to(item));
		chronological_list.push_front(item);

		return &item.GetData();
	}

	/**
	 * Insert a new item into the cache.  The key must not exist
	 * already, i.e. Get() has returned nullptr; it is not
	 * possible to replace an existing item.  If the cache is
	 * full, then the least recently used item is deleted, making
	 * room for this one.
	 */
	template<typename K, typename U>
	void Put(K &&key, U &&data) {
		assert(map.find(key, map.hash_function(), map.key_eq()) == map.end());

		Item &item = Make(std::forward<K>(key), std::forward<U>(data));
		chronological_list.push_front(item);
		map.insert(item);
	}
};

#endif
