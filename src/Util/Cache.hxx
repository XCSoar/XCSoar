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

#include <assert.h>

template<typename Key, typename Data,
	 std::size_t capacity,
	 typename Hash=std::hash<Key>,
	 typename Equal=std::equal_to<Key>>
class Cache {

	class Item
		: public boost::intrusive::unordered_set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>,
		  public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>> {

		Manual<Key> key;

		Manual<Data> data;

	public:
		const Key &GetKey() const {
			return key.Get();
		}

		const Data &GetData() const {
			return data.Get();
		}

		template<typename K, typename U>
		void Construct(K &&_key, U &&value) {
			key.Construct(std::forward<K>(_key));
			data.Construct(std::forward<U>(value));
		}

		void Destruct() {
			key.Destruct();
			data.Destruct();
		}

		template<typename K, typename U>
		void Replace(K &&_key, U &&value) {
			key.Get() = std::forward<K>(_key);
			data.Get() = std::forward<U>(value);
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
	 * The number of cached items.
	 */
	std::size_t size = 0;

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
	KeyMap map;

	Item buffer[capacity];

	static constexpr std::size_t N_BUCKETS = capacity;
	typename KeyMap::bucket_type buckets[N_BUCKETS];

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
		:map(typename KeyMap::bucket_traits(buckets, N_BUCKETS)) {
		for (std::size_t i = 0; i < capacity; ++i)
			unallocated_list.push_back(buffer[i]);
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
		auto i = map.find(key, map.hash_function(), map.key_eq());
		if (i == map.end())
			return nullptr;

		Item &item = *i;

		/* move to the front of the chronological list */
		chronological_list.erase(chronological_list.iterator_to(item));
		chronological_list.push_front(item);

		return &item.GetData();
	}

	template<typename K, typename U>
	void Put(K &&key, U &&data) {
		assert(map.find(key, map.hash_function(), map.key_eq()) == map.end());

		Item &item = Make(std::forward<K>(key), std::forward<U>(data));
		chronological_list.push_front(item);
		map.insert(item);

#ifndef NDEBUG
		++size;
#endif
	}
};

#endif
