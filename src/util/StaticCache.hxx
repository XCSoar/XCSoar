// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Cast.hxx"
#include "Manual.hxx"
#include "IntrusiveHashSet.hxx"
#include "IntrusiveList.hxx"

#include <array>
#include <cassert>
#include <concepts>

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
class StaticCache {

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
		: public IntrusiveHashSetHook<IntrusiveHookMode::NORMAL>,
		  public IntrusiveListHook<IntrusiveHookMode::NORMAL>
	{

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

	using ItemList = IntrusiveList<Item>;

	/**
	 * The list of unallocated items.
	 */
	ItemList unallocated_list;

	ItemList chronological_list;

	using KeyMap = IntrusiveHashSet<Item, table_size, ItemHash, ItemEqual>;

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

		return unallocated_list.pop_front();
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

	StaticCache() noexcept {
		for (auto &i : buffer)
			unallocated_list.push_back(i);
	}

	~StaticCache() noexcept {
		Clear();
	}

	StaticCache(const StaticCache &) = delete;
	StaticCache &operator=(const StaticCache &) = delete;

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
		auto i = map.find(std::forward<K>(key));
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
		assert(map.find(key) == map.end() &&
		       "Key must not exist already");

		Item &item = Make(std::forward<K>(key), std::forward<U>(data));
		chronological_list.push_front(item);
		map.insert(item);
		return item.GetData();
	}

	/**
	 * Insert a new item into the cache.  If the key exists
	 * already, then the item is replaced.
	 */
	template<typename K, typename U>
	Data &PutOrReplace(K &&key, U &&data) {
		auto [position, inserted] = map.insert_check(key);
		if (inserted) {
			Item &item = Make(std::forward<K>(key), std::forward<U>(data));
			chronological_list.push_front(item);
			map.insert_commit(position, item);
			return item.GetData();
		} else {
			position->ReplaceData(std::forward<U>(data));
			return position->GetData();
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
		auto i = map.find(std::forward<K>(key));
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
	void RemoveIf(std::predicate<const Key &, const Data &> auto p) noexcept {
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
	void ForEach(std::invocable<const Key &, const Data &> auto f) const {
		for (const auto &i : chronological_list)
			f(i.GetKey(), i.GetData());
	}
};
