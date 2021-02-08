/*
 * Copyright 2011-2021 Max Kellermann <max.kellermann@gmail.com>
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
#include "Cast.hxx"

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>

#include <array>

#include <cassert>

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

		static constexpr Pair &Cast(Data &data) {
			return ContainerCast(data, &Pair::data);
		}

		template<typename U>
		void ReplaceData(U &&_data) {
			data = std::forward<U>(_data);
		}

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
		static constexpr Item &Cast(Data &data) {
			return ContainerCast(Manual<Pair>::Cast(Pair::Cast(data)),
					     &Item::pair);
		}

		const Key &GetKey() const noexcept {
			return pair->key;
		}

		const Data &GetData() const noexcept {
			return pair->data;
		}

		Data &GetData() noexcept {
			return pair->data;
		}

		template<typename K, typename U>
		void Construct(K &&_key, U &&value) {
			pair.Construct(std::forward<K>(_key),
				       std::forward<U>(value));
		}

		void Destruct() noexcept {
			pair.Destruct();
		}

		template<typename U>
		void ReplaceData(U &&value) {
			pair->ReplaceData(std::forward<U>(value));
		}

		template<typename K, typename U>
		void Replace(K &&_key, U &&value) {
			pair->Replace(std::forward<K>(_key),
				      std::forward<U>(value));
		}
	};

	struct ItemHash : Hash {
		using Hash::operator();

		[[gnu::pure]]
		std::size_t operator()(const Item &a) const noexcept {
			return Hash::operator()(a.GetKey());
		}
	};

	struct ItemEqual : Equal {
		using Equal::operator();

		[[gnu::pure]]
		bool operator()(const Item &a, const Item &b) const noexcept {
			return Equal::operator()(a.GetKey(), b.GetKey());
		}

		template<typename A>
		[[gnu::pure]]
		bool operator()(A &&a, const Item &b) const noexcept {
			return Equal::operator()(std::forward<A>(a), b.GetKey());
		}
	};

	typedef boost::intrusive::list<Item,
				       boost::intrusive::constant_time_size<false>> ItemList;

	/**
	 * The list of unallocated items.
	 */
	ItemList unallocated_list;

	ItemList chronological_list;

	using KeyMap = boost::intrusive::unordered_set<Item,
						       boost::intrusive::hash<ItemHash>,
						       boost::intrusive::equal<ItemEqual>,
						       boost::intrusive::constant_time_size<false>>;

	std::array<typename KeyMap::bucket_type, table_size> buckets;

	KeyMap map;

	std::array<Item, max_size> buffer;

	[[gnu::pure]]
	Item &GetOldest() noexcept {
		assert(!chronological_list.empty());

		return chronological_list.back();
	}

	/**
	 * Remove the oldest item from the cache (both from the #map and
	 * from #chronological_list), but do not destruct it.
	 */
	Item &RemoveOldest() noexcept {
		Item &item = GetOldest();

		map.erase(map.iterator_to(item));
		chronological_list.erase(chronological_list.iterator_to(item));

		return item;
	}

	/**
	 * Allocate an item from #unallocated_list, but do not construct it.
	 */
	Item &Allocate() noexcept {
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
	using hasher = typename KeyMap::hasher;
	using key_equal = typename KeyMap::key_equal;

	Cache() noexcept
		:map(typename KeyMap::bucket_traits(&buckets.front(), buckets.size())) {
		for (auto &i : buffer)
			unallocated_list.push_back(i);
	}

	~Cache() noexcept {
		Clear();
	}

	Cache(const Cache &) = delete;
	Cache &operator=(const Cache &) = delete;

	decltype(auto) hash_function() const noexcept {
		return map.hash_function();
	}

	decltype(auto) key_eq() const noexcept {
		return map.key_eq();
	}

	bool IsEmpty() const noexcept {
		return chronological_list.empty();
	}

	bool IsFull() const noexcept {
		return unallocated_list.empty();
	}

	void Clear() noexcept {
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
	template<typename K>
	[[gnu::pure]]
	Data *Get(K &&key) noexcept {
		auto i = map.find(std::forward<K>(key),
				  map.hash_function(), map.key_eq());
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
	Data &Put(K &&key, U &&data) {
		Item &item = Make(std::forward<K>(key), std::forward<U>(data));
		chronological_list.push_front(item);
		auto i = map.insert(item);
		(void)i;
		assert(i.second && "Key must not exist already");
		return item.GetData();
	}

	/**
	 * Insert a new item into the cache.  If the key exists
	 * already, then the item is replaced.
	 */
	template<typename K, typename U>
	Data &PutOrReplace(K &&key, U &&data) {
		typename KeyMap::insert_commit_data icd;
		auto i = map.insert_check(key,
					  map.hash_function(), map.key_eq(),
					  icd);
		if (i.second) {
			Item &item = Make(std::forward<K>(key), std::forward<U>(data));
			chronological_list.push_front(item);
			map.insert_commit(item, icd);
			return item.GetData();
		} else {
			i.first->ReplaceData(std::forward<U>(data));
			return i.first->GetData();
		}
	}

	/**
	 * Remove an item from the cache using a reference to the
	 * value.
	 */
	void RemoveItem(Data &data) noexcept {
		auto &item = Item::Cast(data);

		map.erase(map.iterator_to(item));
		chronological_list.erase(chronological_list.iterator_to(item));

		item.Destruct();
		unallocated_list.push_front(item);
	}

	/**
	 * Remove an item from the cache.
	 */
	template<typename K>
	void Remove(K &&key) noexcept {
		auto i = map.find(std::forward<K>(key),
				  map.hash_function(), map.key_eq());
		if (i == map.end())
			return;

		Item &item = *i;

		map.erase(i);
		chronological_list.erase(chronological_list.iterator_to(item));

		item.Destruct();
		unallocated_list.push_front(item);
	}

	/**
	 * Iterates over all items and remove all those which match
	 * the given predicate.
	 */
	template<typename P>
	void RemoveIf(P &&p) noexcept {
		chronological_list.remove_and_dispose_if([&p](const Item &item){
				return p(item.GetKey(), item.GetData());
			},
			[this](Item *item){
				map.erase(map.iterator_to(*item));
				item->Destruct();
				unallocated_list.push_front(*item);
			});
	}

	/**
	 * Iterates over all items, passing each key/value pair to a
	 * given function.  The cache must not be modified from within
	 * that function.
	 */
	template<typename F>
	void ForEach(F &&f) const {
		for (const auto &i : chronological_list)
			f(i.GetKey(), i.GetData());
	}
};

#endif
