/*
 * Copyright 2020-2022 Max Kellermann <max.kellermann@gmail.com>
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

#include "Cast.hxx"
#include "Concepts.hxx"
#include "IntrusiveHookMode.hxx"
#include "MemberPointer.hxx"
#include "OptionalCounter.hxx"

#include <iterator>
#include <type_traits>
#include <utility>

struct IntrusiveListNode {
	IntrusiveListNode *next, *prev;

	static constexpr void Connect(IntrusiveListNode &a,
				      IntrusiveListNode &b) noexcept {
		a.next = &b;
		b.prev = &a;
	}
};

template<IntrusiveHookMode _mode=IntrusiveHookMode::NORMAL>
class IntrusiveListHook {
	template<typename T> friend struct IntrusiveListBaseHookTraits;
	template<auto member> friend struct IntrusiveListMemberHookTraits;
	template<typename T, typename HookTraits, bool> friend class IntrusiveList;

protected:
	IntrusiveListNode siblings;

public:
	static constexpr IntrusiveHookMode mode = _mode;

	IntrusiveListHook() noexcept {
		if constexpr (mode >= IntrusiveHookMode::TRACK)
			siblings.next = nullptr;
	}

	~IntrusiveListHook() noexcept {
		if constexpr (mode >= IntrusiveHookMode::AUTO_UNLINK)
			if (is_linked())
				unlink();
	}

	IntrusiveListHook(const IntrusiveListHook &) = delete;
	IntrusiveListHook &operator=(const IntrusiveListHook &) = delete;

	void unlink() noexcept {
		IntrusiveListNode::Connect(*siblings.prev, *siblings.next);

		if constexpr (mode >= IntrusiveHookMode::TRACK)
			siblings.next = nullptr;
	}

	bool is_linked() const noexcept {
		static_assert(mode >= IntrusiveHookMode::TRACK);

		return siblings.next != nullptr;
	}

private:
	static constexpr auto &Cast(IntrusiveListNode &node) noexcept {
		return ContainerCast(node, &IntrusiveListHook::siblings);
	}

	static constexpr const auto &Cast(const IntrusiveListNode &node) noexcept {
		return ContainerCast(node, &IntrusiveListHook::siblings);
	}
};

using SafeLinkIntrusiveListHook =
	IntrusiveListHook<IntrusiveHookMode::TRACK>;
using AutoUnlinkIntrusiveListHook =
	IntrusiveListHook<IntrusiveHookMode::AUTO_UNLINK>;

/**
 * Detect the hook type which is embedded in the given type as a base
 * class.  This is a template to postpone the type checks, to allow
 * forward-declared types.
 */
template<typename U>
struct IntrusiveListHookDetection {
	/* TODO can this be simplified somehow, without checking for
	   all possible enum values? */
	using type = std::conditional_t<std::is_base_of_v<IntrusiveListHook<IntrusiveHookMode::NORMAL>, U>,
					IntrusiveListHook<IntrusiveHookMode::NORMAL>,
					std::conditional_t<std::is_base_of_v<IntrusiveListHook<IntrusiveHookMode::TRACK>, U>,
							   IntrusiveListHook<IntrusiveHookMode::TRACK>,
							   std::conditional_t<std::is_base_of_v<IntrusiveListHook<IntrusiveHookMode::AUTO_UNLINK>, U>,
									      IntrusiveListHook<IntrusiveHookMode::AUTO_UNLINK>,
									      void>>>;
};

/**
 * For classes which embed #IntrusiveListHook as base class.
 */
template<typename T>
struct IntrusiveListBaseHookTraits {
	template<typename U>
	using Hook = typename IntrusiveListHookDetection<U>::type;

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		auto *hook = &Hook<T>::Cast(*node);
		return static_cast<T *>(hook);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return static_cast<Hook<T> &>(t);
	}
};

/**
 * For classes which embed #IntrusiveListHook as member.
 */
template<auto member>
struct IntrusiveListMemberHookTraits {
	using T = MemberPointerContainerType<decltype(member)>;
	using _Hook = MemberPointerType<decltype(member)>;

	template<typename Dummy>
	using Hook = _Hook;

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		auto &hook = Hook<T>::Cast(*node);
		return &ContainerCast(hook, member);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return t.*member;
	}
};

/**
 * @param constant_time_size make size() constant-time by caching the
 * number of items in a field?
 */
template<typename T,
	 typename HookTraits=IntrusiveListBaseHookTraits<T>,
	 bool constant_time_size=false>
class IntrusiveList {
	IntrusiveListNode head{&head, &head};

	[[no_unique_address]]
	OptionalCounter<constant_time_size> counter;

	static constexpr auto GetHookMode() noexcept {
		return HookTraits::template Hook<T>::mode;
	}

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		return HookTraits::Cast(node);
	}

	static constexpr const T *Cast(const IntrusiveListNode *node) noexcept {
		return HookTraits::Cast(const_cast<IntrusiveListNode *>(node));
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return HookTraits::ToHook(t);
	}

	static constexpr const auto &ToHook(const T &t) noexcept {
		return HookTraits::ToHook(const_cast<T &>(t));
	}

	static constexpr IntrusiveListNode &ToNode(T &t) noexcept {
		return ToHook(t).siblings;
	}

	static constexpr const IntrusiveListNode &ToNode(const T &t) noexcept {
		return ToHook(t).siblings;
	}

public:
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *;
	using size_type = std::size_t;

	constexpr IntrusiveList() noexcept = default;

	IntrusiveList(IntrusiveList &&src) noexcept {
		if (src.empty())
			return;

		head = src.head;
		head.next->prev = &head;
		head.prev->next = &head;

		src.head.next = &src.head;
		src.head.prev = &src.head;

		using std::swap;
		swap(counter, src.counter);
	}

	~IntrusiveList() noexcept {
		if constexpr (GetHookMode() >= IntrusiveHookMode::TRACK)
			clear();
	}

	IntrusiveList &operator=(IntrusiveList &&) = delete;

	friend void swap(IntrusiveList &a, IntrusiveList &b) noexcept {
		using std::swap;

		if (a.empty()) {
			if (b.empty())
				return;

			a.head = b.head;
			a.head.next->prev = &a.head;
			a.head.prev->next = &a.head;

			b.head = {&b.head, &b.head};
		} else if (b.empty()) {
			b.head = a.head;
			b.head.next->prev = &b.head;
			b.head.prev->next = &b.head;

			a.head = {&a.head, &a.head};
		} else {
			swap(a.head, b.head);

			a.head.next->prev = &a.head;
			a.head.prev->next = &a.head;

			b.head.next->prev = &b.head;
			b.head.prev->next = &b.head;
		}

		swap(a.counter, b.counter);
	}

	constexpr bool empty() const noexcept {
		return head.next == &head;
	}

	constexpr size_type size() const noexcept {
		if constexpr (constant_time_size)
			return counter;
		else
			return std::distance(begin(), end());
	}

	void clear() noexcept {
		if constexpr (GetHookMode() >= IntrusiveHookMode::TRACK) {
			/* for SafeLinkIntrusiveListHook, we need to
			   remove each item manually, or else its
			   is_linked() method will not work */
			while (!empty())
				pop_front();
		} else {
			head = {&head, &head};
			counter.reset();
		}
	}

	void clear_and_dispose(Disposer<value_type> auto disposer) noexcept {
		while (!empty()) {
			auto *item = &front();
			pop_front();
			disposer(item);
		}
	}

	/**
	 * @return the number of removed items
	 */
	std::size_t remove_and_dispose_if(Predicate<const_reference> auto pred,
					  Disposer<value_type> auto dispose) noexcept {
		std::size_t result = 0;

		auto *n = head.next;

		while (n != &head) {
			auto *i = Cast(n);
			n = n->next;

			if (pred(*i)) {
				ToHook(*i).unlink();
				--counter;
				dispose(i);
				++result;
			}
		}

		return result;
	}

	const_reference front() const noexcept {
		return *Cast(head.next);
	}

	reference front() noexcept {
		return *Cast(head.next);
	}

	void pop_front() noexcept {
		ToHook(front()).unlink();
		--counter;
	}

	void pop_front_and_dispose(Disposer<value_type> auto disposer) noexcept {
		auto &i = front();
		ToHook(i).unlink();
		--counter;
		disposer(&i);
	}

	reference back() noexcept {
		return *Cast(head.prev);
	}

	void pop_back() noexcept {
		ToHook(back()).unlink();
		--counter;
	}

	class const_iterator;

	class iterator final {
		friend IntrusiveList;
		friend const_iterator;

		IntrusiveListNode *cursor;

		constexpr iterator(IntrusiveListNode *_cursor) noexcept
			:cursor(_cursor) {}

	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		iterator() noexcept = default;

		constexpr bool operator==(const iterator &other) const noexcept {
			return cursor == other.cursor;
		}

		constexpr bool operator!=(const iterator &other) const noexcept {
			return !(*this == other);
		}

		constexpr reference operator*() const noexcept {
			return *Cast(cursor);
		}

		constexpr pointer operator->() const noexcept {
			return Cast(cursor);
		}

		auto &operator++() noexcept {
			cursor = cursor->next;
			return *this;
		}

		auto operator++(int) noexcept {
			auto old = *this;
			cursor = cursor->next;
			return old;
		}

		auto &operator--() noexcept {
			cursor = cursor->prev;
			return *this;
		}

		auto operator--(int) noexcept {
			auto old = *this;
			cursor = cursor->prev;
			return old;
		}
	};

	constexpr iterator begin() noexcept {
		return {head.next};
	}

	constexpr iterator end() noexcept {
		return {&head};
	}

	static constexpr iterator iterator_to(reference t) noexcept {
		return {&ToNode(t)};
	}

	class const_iterator final {
		friend IntrusiveList;

		const IntrusiveListNode *cursor;

		constexpr const_iterator(const IntrusiveListNode *_cursor) noexcept
			:cursor(_cursor) {}

	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = const T;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using reference = value_type &;

		const_iterator() noexcept = default;

		const_iterator(iterator src) noexcept
			:cursor(src.cursor) {}

		constexpr bool operator==(const const_iterator &other) const noexcept {
			return cursor == other.cursor;
		}

		constexpr bool operator!=(const const_iterator &other) const noexcept {
			return !(*this == other);
		}

		constexpr reference operator*() const noexcept {
			return *Cast(cursor);
		}

		constexpr pointer operator->() const noexcept {
			return Cast(cursor);
		}

		auto &operator++() noexcept {
			cursor = cursor->next;
			return *this;
		}

		auto operator++(int) noexcept {
			auto old = *this;
			cursor = cursor->next;
			return old;
		}

		auto &operator--() noexcept {
			cursor = cursor->prev;
			return *this;
		}

		auto operator--(int) noexcept {
			auto old = *this;
			cursor = cursor->prev;
			return old;
		}
	};

	constexpr const_iterator begin() const noexcept {
		return {head.next};
	}

	constexpr const_iterator end() const noexcept {
		return {&head};
	}

	static constexpr const_iterator iterator_to(const_reference t) noexcept {
		return {&ToNode(t)};
	}

	iterator erase(iterator i) noexcept {
		auto result = std::next(i);
		ToHook(*i).unlink();
		--counter;
		return result;
	}

	iterator erase_and_dispose(iterator i,
				   Disposer<value_type> auto disposer) noexcept {
		auto result = erase(i);
		disposer(&*i);
		return result;
	}

	void push_front(reference t) noexcept {
		insert(begin(), t);
	}

	void push_back(reference t) noexcept {
		insert(end(), t);
	}

	void insert(iterator p, reference t) noexcept {
		static_assert(!constant_time_size ||
			      GetHookMode() < IntrusiveHookMode::AUTO_UNLINK,
			      "Can't use auto-unlink hooks with constant_time_size");

		auto &existing_node = ToNode(*p);
		auto &new_node = ToNode(t);

		IntrusiveListNode::Connect(*existing_node.prev,
					   new_node);
		IntrusiveListNode::Connect(new_node, existing_node);

		++counter;
	}

	/**
	 * Move one item of the given list to this one before the
	 * given position.
	 */
	void splice(iterator position,
		    IntrusiveList &from, iterator i) noexcept {
		auto &item = *i;
		from.erase(i);
		insert(position, item);
	}

	/**
	 * Move the given range of items of the given list to this one
	 * before the given position.
	 */
	void splice(iterator position, IntrusiveList &from,
		    iterator _begin, iterator _end, size_type n) noexcept {
		if (_begin == _end)
			return;

		auto &next_node = ToNode(*position);
		auto &prev_node = ToNode(*std::prev(position));

		auto &first_node = ToNode(*_begin);
		auto &before_first_node = ToNode(*std::prev(_begin));
		auto &last_node = ToNode(*std::prev(_end));
		auto &after_last_node = ToNode(*_end);

		/* remove from the other list */
		IntrusiveListNode::Connect(before_first_node, after_last_node);
		from.counter -= n;

		/* insert into this list */
		IntrusiveListNode::Connect(prev_node, first_node);
		IntrusiveListNode::Connect(last_node, next_node);
		counter += n;
	}

	/**
	 * Move all items of the given list to this one before the
	 * given position.
	 */
	void splice(iterator position, IntrusiveList &from) noexcept {
		splice(position, from, from.begin(), from.end(),
		       constant_time_size ? from.size() : 1);
	}
};
