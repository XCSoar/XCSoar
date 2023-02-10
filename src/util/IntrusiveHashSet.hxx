/*
 * Copyright 2022 Max Kellermann <max.kellermann@gmail.com>
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

#pragma once

#include "Concepts.hxx"
#include "IntrusiveList.hxx"

#include <algorithm> // for std::all_of()
#include <array>
#include <numeric> // for std::accumulate()

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
 * Detect the hook type.
 */
template<typename U>
struct IntrusiveHashSetHookDetection {
	/* TODO can this be simplified somehow, without checking for
	   all possible enum values? */
	using type = std::conditional_t<std::is_base_of_v<IntrusiveHashSetHook<IntrusiveHookMode::NORMAL>, U>,
					IntrusiveHashSetHook<IntrusiveHookMode::NORMAL>,
					std::conditional_t<std::is_base_of_v<IntrusiveHashSetHook<IntrusiveHookMode::TRACK>, U>,
							   IntrusiveHashSetHook<IntrusiveHookMode::TRACK>,
							   std::conditional_t<std::is_base_of_v<IntrusiveHashSetHook<IntrusiveHookMode::AUTO_UNLINK>, U>,
									      IntrusiveHashSetHook<IntrusiveHookMode::AUTO_UNLINK>,
									      void>>>;
};

/**
 * For classes which embed #IntrusiveHashSetHook as base class.
 */
template<typename T>
struct IntrusiveHashSetBaseHookTraits {
	template<typename U>
	using Hook = typename IntrusiveHashSetHookDetection<U>::type;

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
 * A hash table implementation which stores pointers to items which
 * have an embedded #IntrusiveHashSetHook.  The actual table is
 * embedded with a compile-time fixed size in this object.
 */
template<typename T, std::size_t table_size,
	 typename Hash=typename T::Hash, typename Equal=typename T::Equal,
	 typename HookTraits=IntrusiveHashSetBaseHookTraits<T>,
	 bool constant_time_size=false>
class IntrusiveHashSet {
	[[no_unique_address]]
	OptionalCounter<constant_time_size> counter;

	[[no_unique_address]]
	Hash hash;

	[[no_unique_address]]
	Equal equal;

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

	using Bucket = IntrusiveList<T, BucketHookTraits>;
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

	using hasher = Hash;
	using key_equal = Equal;

	[[nodiscard]]
	IntrusiveHashSet() noexcept = default;

	[[nodiscard]]
	constexpr const hasher &hash_function() const noexcept {
		return hash;
	}

	[[nodiscard]]
	constexpr const key_equal &key_eq() const noexcept {
		return equal;
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

	void remove_and_dispose_if(Predicate<const_reference> auto pred,
				   Disposer<value_type> auto disposer) noexcept {
		for (auto &bucket : table)
			counter -= bucket.remove_and_dispose_if(pred, disposer);
	}

	constexpr void remove_and_dispose_if(const auto &key,
					     Predicate<const_reference> auto pred,
					     Disposer<value_type> auto disposer) noexcept {
		auto &bucket = GetBucket(key);
		counter -= bucket.remove_and_dispose_if([this, &key, &pred](const auto &item){
			return equal(key, item) && pred(item);
		}, disposer);
	}

	[[nodiscard]]
	static constexpr bucket_iterator iterator_to(reference item) noexcept {
		return Bucket::iterator_to(item);
	}

	[[nodiscard]] [[gnu::pure]]
	constexpr std::pair<bucket_iterator, bool> insert_check(const auto &key) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (equal(key, i))
				return {bucket.iterator_to(i), false};

		return {bucket.begin(), true};
	}

	constexpr void insert(bucket_iterator bucket, reference item) noexcept {
		++counter;
		GetBucket(item).insert(bucket, item);
	}

	constexpr void insert(reference item) noexcept {
		++counter;
		GetBucket(item).push_front(item);
	}

	constexpr bucket_iterator erase(bucket_iterator i) noexcept {
		--counter;
		return GetBucket(*i).erase(i);
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
			if (equal(key, i))
				return bucket.iterator_to(i);

		return end();
	}

	[[nodiscard]] [[gnu::pure]]
	constexpr const_bucket_iterator find(const auto &key) const noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (equal(key, i))
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
					  Predicate<const_reference> auto pred) noexcept {
		auto &bucket = GetBucket(key);
		for (auto &i : bucket)
			if (equal(key, i) && pred(i))
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
						 Predicate<const_reference> auto expired_pred,
						 Disposer<value_type> auto disposer,
						 Predicate<const_reference> auto match_pred) noexcept {
		auto &bucket = GetBucket(key);

		for (auto i = bucket.begin(), e = bucket.end(); i != e;) {
			if (!equal(key, *i))
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
		const auto h = hash(std::forward<K>(key));
		return table[h % table_size];
	}

	template<typename K>
	[[gnu::pure]]
	[[nodiscard]]
	constexpr const auto &GetBucket(K &&key) const noexcept {
		const auto h = hash(std::forward<K>(key));
		return table[h % table_size];
	}
};
