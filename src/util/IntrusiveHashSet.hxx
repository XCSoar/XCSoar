// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Concepts.hxx"
#include "IntrusiveList.hxx"

#include <algorithm> // for std::all_of()
#include <array>
#include <numeric> // for std::accumulate()

struct IntrusiveHashSetOptions {
	bool constant_time_size = false;

	/**
	 * @see IntrusiveListOptions::zero_initialized
	 */
	bool zero_initialized = false;
};

template<IntrusiveHookMode mode=IntrusiveHookMode::NORMAL>
struct IntrusiveHashSetHook {
	using SiblingsHook = IntrusiveListHook<mode>;

	SiblingsHook intrusive_hash_set_siblings;

	void unlink() noexcept {
		intrusive_hash_set_siblings.unlink();
	}

	bool is_linked() const noexcept {
		return intrusive_hash_set_siblings.is_linked();
	}
};

/**
 * For classes which embed #IntrusiveHashSetHook as base class.
 */
template<typename T>
struct IntrusiveHashSetBaseHookTraits {
	/* a never-called helper function which is used by _Cast() */
	template<IntrusiveHookMode mode>
	static constexpr IntrusiveHashSetHook<mode> _Identity(const IntrusiveHashSetHook<mode> &) noexcept;

	/* another never-called helper function which "calls"
	   _Identity(), implicitly casting the item to the
	   IntrusiveHashSetHook specialization; we use this to detect
	   which IntrusiveHashSetHook specialization is used */
	template<typename U>
	static constexpr auto _Cast(const U &u) noexcept {
		return decltype(_Identity(u))();
	}

	template<typename U>
	using Hook = decltype(_Cast(std::declval<U>()));

	static constexpr T *Cast(Hook<T> *node) noexcept {
		return static_cast<T *>(node);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return static_cast<Hook<T> &>(t);
	}
};

/**
 * For classes which embed #IntrusiveListHook as member.
 */
template<auto member>
struct IntrusiveHashSetMemberHookTraits {
	using T = MemberPointerContainerType<decltype(member)>;
	using _Hook = MemberPointerType<decltype(member)>;

	template<typename Dummy>
	using Hook = _Hook;

	static constexpr T *Cast(Hook<T> *node) noexcept {
		return &ContainerCast(*node, member);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return t.*member;
	}
};

/**
 * @param GetKey a function object which extracts the "key" part of an
 * item
 */
template<typename Hash, typename Equal,
	 typename GetKey=std::identity>
struct IntrusiveHashSetOperators {
	using hasher = Hash;
	using key_equal = Equal;

	[[no_unique_address]]
	Hash hash;

	[[no_unique_address]]
	Equal equal;

	[[no_unique_address]]
	GetKey get_key;
};

/**
 * A hash table implementation which stores pointers to items which
 * have an embedded #IntrusiveHashSetHook.  The actual table is
 * embedded with a compile-time fixed size in this object.
 *
 * @param Operators a class which contains functions `hash` and
 * `equal`
 */
template<typename T, std::size_t table_size,
	 typename Operators,
	 typename HookTraits=IntrusiveHashSetBaseHookTraits<T>,
	 IntrusiveHashSetOptions options=IntrusiveHashSetOptions{}>
class IntrusiveHashSet {
	static constexpr bool constant_time_size = options.constant_time_size;

	[[no_unique_address]]
	OptionalCounter<constant_time_size> counter;

	[[no_unique_address]]
	Operators ops;

	struct BucketHookTraits {
		template<typename U>
		using HashSetHook = typename HookTraits::template Hook<U>;

		template<typename U>
		using ListHook = IntrusiveListMemberHookTraits<&HashSetHook<U>::intrusive_hash_set_siblings>;

		template<typename U>
		using Hook = typename HashSetHook<U>::SiblingsHook;

		static constexpr T *Cast(IntrusiveListNode *node) noexcept {
			auto *hook = ListHook<T>::Cast(node);
			return HookTraits::Cast(hook);
		}

		static constexpr auto &ToHook(T &t) noexcept {
			auto &hook = HookTraits::ToHook(t);
			return hook.intrusive_hash_set_siblings;
		}
	};

	using Bucket = IntrusiveList<T, BucketHookTraits, IntrusiveListOptions{.zero_initialized = options.zero_initialized}>;
	std::array<Bucket, table_size> table;

	using bucket_iterator = typename Bucket::iterator;
	using const_bucket_iterator = typename Bucket::const_iterator;

public:
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *;
	using size_type = std::size_t;

	using hasher = typename Operators::hasher;
	using key_equal = typename Operators::key_equal;

	[[nodiscard]]
	IntrusiveHashSet() noexcept = default;

	[[nodiscard]]
	constexpr const hasher &hash_function() const noexcept {
		return ops.hash;
	}

	[[nodiscard]]
	constexpr const key_equal &key_eq() const noexcept {
		return ops.equal;
	}

	[[nodiscard]]
	constexpr bool empty() const noexcept {
		if constexpr (constant_time_size)
			return size() == 0;
		else
			return std::all_of(table.begin(), table.end(), [](const auto &bucket){
				return bucket.empty();
			});
	}

	[[nodiscard]]
	constexpr size_type size() const noexcept {
		if constexpr (constant_time_size)
			return counter;
		else
			return std::accumulate(table.begin(), table.end(), size_type{}, [](std::size_t n, const auto &bucket){
				return n + bucket.size();
			});
	}

	constexpr void clear() noexcept {
		for (auto &i : table)
			i.clear();

		counter.reset();
	}

	constexpr void clear_and_dispose(Disposer<value_type> auto disposer) noexcept {
		for (auto &i : table)
			i.clear_and_dispose(disposer);

		counter.reset();
	}

	/**
	 * Remove and dispose all items matching the given predicate.
	 *
	 * @return the number of removed items
	 */
	std::size_t remove_and_dispose_if(std::predicate<const_reference> auto pred,
				   Disposer<value_type> auto disposer) noexcept {
		std::size_t n = 0;
		for (auto &bucket : table)
			n += bucket.remove_and_dispose_if(pred, disposer);
		counter -= n;
		return n;
	}

	/**
	 * Remove and dispose all items with the specified key.
	 *
	 * @return the number of removed items
	 */
	constexpr std::size_t remove_and_dispose_key(const auto &key,
						     Disposer<value_type> auto disposer) noexcept {
		auto &bucket = GetBucket(key);
		std::size_t n = bucket.remove_and_dispose_if([this, &key](const auto &item){
			return ops.equal(key, ops.get_key(item));
		}, disposer);
		counter -= n;
		return n;
	}

	constexpr std::size_t remove_and_dispose_key_if(const auto &key,
							std::predicate<const_reference> auto pred,
							Disposer<value_type> auto disposer) noexcept {
		auto &bucket = GetBucket(key);
		std::size_t n = bucket.remove_and_dispose_if([this, &key, &pred](const auto &item){
			return ops.equal(key, ops.get_key(item)) && pred(item);
		}, disposer);
		counter -= n;
		return n;
	}

	[[nodiscard]]
	static constexpr bucket_iterator iterator_to(reference item) noexcept {
		return Bucket::iterator_to(item);
	}

	/**
	 * Prepare insertion of a new item.  If the key already
	 * exists, return an iterator to the existing item and
	 * `false`.  If the key does not exist, return an iterator to
	 * the bucket where the new item may be inserted using
	 * insert() and `true`.
	 */
	[[nodiscard]] [[gnu::pure]]
	constexpr std::pair<bucket_iterator, bool> insert_check(const auto &key) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (ops.equal(key, ops.get_key(i)))
				return {bucket.iterator_to(i), false};

		/* bucket.end() is a pointer to the bucket's list
		   head, a stable value that is guaranteed to be still
		   valid when insert_commit() gets called
		   eventually */
		return {bucket.end(), true};
	}

	/**
	 * Like insert_check(), but existing items are only considered
	 * conflicting if they match the given predicate.
	 */
	[[nodiscard]] [[gnu::pure]]
	constexpr std::pair<bucket_iterator, bool> insert_check_if(const auto &key,
								   std::predicate<const_reference> auto pred) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (ops.equal(key, ops.get_key(i)) && pred(i))
				return {bucket.iterator_to(i), false};

		/* bucket.end() is a pointer to the bucket's list
		   head, a stable value that is guaranteed to be still
		   valid when insert_commit() gets called
		   eventually */
		return {bucket.end(), true};
	}

	/**
	 * Finish the insertion if insert_check() has returned true.
	 *
	 * @param bucket the bucket returned by insert_check()
	 */
	constexpr void insert_commit(bucket_iterator bucket, reference item) noexcept {
		++counter;

		/* using insert_after() so the new item gets inserted
		   at the front of the bucket list */
		GetBucket(ops.get_key(item)).insert_after(bucket, item);
	}

	/**
	 * Insert a new item without checking whether the key already
	 * exists.
	 */
	constexpr void insert(reference item) noexcept {
		++counter;
		GetBucket(ops.get_key(item)).push_front(item);
	}

	constexpr bucket_iterator erase(bucket_iterator i) noexcept {
		--counter;
		return GetBucket(ops.get_key(*i)).erase(i);
	}

	constexpr bucket_iterator erase_and_dispose(bucket_iterator i,
						    Disposer<value_type> auto disposer) noexcept {
		auto result = erase(i);
		disposer(&*i);
		return result;
	}

	[[nodiscard]] [[gnu::pure]]
	constexpr bucket_iterator find(const auto &key) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (ops.equal(key, ops.get_key(i)))
				return bucket.iterator_to(i);

		return end();
	}

	[[nodiscard]] [[gnu::pure]]
	constexpr const_bucket_iterator find(const auto &key) const noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (ops.equal(key, ops.get_key(i)))
				return bucket.iterator_to(i);

		return end();
	}

	/**
	 * Like find(), but returns an item that matches the given
	 * predicate.  This is useful if the container can contain
	 * multiple items that compare equal (according to #Equal, but
	 * not according to #pred).
	 */
	[[nodiscard]] [[gnu::pure]]
	constexpr bucket_iterator find_if(const auto &key,
					  std::predicate<const_reference> auto pred) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (ops.equal(key, ops.get_key(i)) && pred(i))
				return bucket.iterator_to(i);

		return end();
	}

	/**
	 * Like find_if(), but while traversing the bucket linked
	 * list, remove and dispose expired items.
	 *
	 * @param expired_pred returns true if an item is expired; it
	 * will be removed and disposed
	 *
	 * @param disposer function which will be called for items
	 * that were removed (because they are expired)
	 *
	 * @param match_pred returns true if the desired item was
	 * found
	 */
	[[nodiscard]] [[gnu::pure]]
	constexpr bucket_iterator expire_find_if(const auto &key,
						 std::predicate<const_reference> auto expired_pred,
						 Disposer<value_type> auto disposer,
						 std::predicate<const_reference> auto match_pred) noexcept {
		auto &bucket = GetBucket(key);

		for (auto i = bucket.begin(), e = bucket.end(); i != e;) {
			if (!ops.equal(key, ops.get_key(*i)))
				++i;
			else if (expired_pred(*i))
				i = erase_and_dispose(i, disposer);
			else if (match_pred(*i))
				return i;
			else
				++i;
		}

		return end();
	}

	constexpr bucket_iterator end() noexcept {
		return table.front().end();
	}

	constexpr const_bucket_iterator end() const noexcept {
		return table.front().end();
	}

	constexpr void for_each(auto &&f) {
		for (auto &bucket : table)
			for (auto &i : bucket)
				f(i);
	}

	constexpr void for_each(auto &&f) const {
		for (const auto &bucket : table)
			for (const auto &i : bucket)
				f(i);
	}

private:
	template<typename K>
	[[gnu::pure]]
	[[nodiscard]]
	constexpr auto &GetBucket(K &&key) noexcept {
		const auto h = ops.hash(std::forward<K>(key));
		return table[h % table_size];
	}

	template<typename K>
	[[gnu::pure]]
	[[nodiscard]]
	constexpr const auto &GetBucket(K &&key) const noexcept {
		const auto h = ops.hash(std::forward<K>(key));
		return table[h % table_size];
	}
};
