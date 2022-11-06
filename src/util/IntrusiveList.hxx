/*
 * Copyright 2020 Max Kellermann <max.kellermann@gmail.com>
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
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
#include "MemberPointer.hxx"
#include "OptionalCounter.hxx"

#include <iterator>
#include <type_traits>
#include <utility>

struct IntrusiveListNode {
	IntrusiveListNode *next, *prev;
};

class IntrusiveListHook {
	template<typename T> friend struct IntrusiveListBaseHookTraits;
	template<auto member> friend struct IntrusiveListMemberHookTraits;
	template<typename T, typename HookTraits, bool> friend class IntrusiveList;

protected:
	IntrusiveListNode siblings;

public:
	IntrusiveListHook() noexcept = default;

	IntrusiveListHook(const IntrusiveListHook &) = delete;
	IntrusiveListHook &operator=(const IntrusiveListHook &) = delete;

	void unlink() noexcept {
		siblings.next->prev = siblings.prev;
		siblings.prev->next = siblings.next;
	}

private:
	static constexpr auto &Cast(IntrusiveListNode &node) noexcept {
		return ContainerCast(node, &IntrusiveListHook::siblings);
	}

	static constexpr const auto &Cast(const IntrusiveListNode &node) noexcept {
		return ContainerCast(node, &IntrusiveListHook::siblings);
	}
};

/**
 * A variant of #IntrusiveListHook which keeps track of whether it is
 * currently in a list.
 */
class SafeLinkIntrusiveListHook : public IntrusiveListHook {
public:
	SafeLinkIntrusiveListHook() noexcept {
		siblings.next = nullptr;
	}

	void unlink() noexcept {
		IntrusiveListHook::unlink();
		siblings.next = nullptr;
	}

	bool is_linked() const noexcept {
		return siblings.next != nullptr;
	}
};

/**
 * A variant of #IntrusiveListHook which auto-unlinks itself from the
 * list upon destruction.  As a side effect, it has an is_linked()
 * method.
 */
class AutoUnlinkIntrusiveListHook : public SafeLinkIntrusiveListHook {
public:
	~AutoUnlinkIntrusiveListHook() noexcept {
		if (is_linked())
			unlink();
	}
};

/**
 * Detect the hook type; this is important because
 * SafeLinkIntrusiveListHook::unlink() needs to clear the "next"
 * pointer.  This is a template to postpone the type checks, to allow
 * forward-declared types.
 */
template<typename U>
struct IntrusiveListHookDetection {
	static_assert(std::is_base_of_v<IntrusiveListHook, U>);

	using type = std::conditional_t<std::is_base_of_v<SafeLinkIntrusiveListHook, U>,
					SafeLinkIntrusiveListHook,
					IntrusiveListHook>;
};

/**
 * For classes which embed #IntrusiveListHook as base class.
 */
template<typename T>
struct IntrusiveListBaseHookTraits {
	template<typename U>
	using Hook = typename IntrusiveListHookDetection<U>::type;

	static constexpr bool IsAutoUnlink() noexcept {
		return std::is_base_of_v<AutoUnlinkIntrusiveListHook, T>;
	}

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		auto *hook = &Hook<T>::Cast(*node);
		return static_cast<T *>(hook);
	}

	static constexpr const T *Cast(const IntrusiveListNode *node) noexcept {
		const auto *hook = &Hook<T>::Cast(*node);
		return static_cast<const T *>(hook);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return static_cast<Hook<T> &>(t);
	}

	static constexpr const auto &ToHook(const T &t) noexcept {
		return static_cast<const Hook<T> &>(t);
	}
};

/**
 * For classes which embed #IntrusiveListHook as member.
 */
template<auto member>
struct IntrusiveListMemberHookTraits {
	using T = MemberPointerContainerType<decltype(member)>;
	using _Hook = MemberPointerType<decltype(member)>;
	using Hook = typename IntrusiveListHookDetection<_Hook>::type;

	static constexpr bool IsAutoUnlink() noexcept {
		return std::is_base_of_v<AutoUnlinkIntrusiveListHook, _Hook>;
	}

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		auto &hook = Hook::Cast(*node);
		return &ContainerCast(hook, member);
	}

	static constexpr const T *Cast(const IntrusiveListNode *node) noexcept {
		const auto &hook = Hook::Cast(*node);
		return &ContainerCast(hook, member);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return t.*member;
	}

	static constexpr const auto &ToHook(const T &t) noexcept {
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
	template<typename U>
	using Hook = typename IntrusiveListHookDetection<U>::type;

	IntrusiveListNode head{&head, &head};

	[[no_unique_address]]
	OptionalCounter<constant_time_size> counter;

	static constexpr T *Cast(IntrusiveListNode *node) noexcept {
		return HookTraits::Cast(node);
	}

	static constexpr const T *Cast(const IntrusiveListNode *node) noexcept {
		return HookTraits::Cast(node);
	}

	static constexpr auto &ToHook(T &t) noexcept {
		return HookTraits::ToHook(t);
	}

	static constexpr const auto &ToHook(const T &t) noexcept {
		return HookTraits::ToHook(t);
	}

	static constexpr IntrusiveListNode &ToNode(T &t) noexcept {
		return ToHook(t).siblings;
	}

	static constexpr const IntrusiveListNode &ToNode(const T &t) noexcept {
		return ToHook(t).siblings;
	}

public:
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
		if constexpr (std::is_base_of_v<SafeLinkIntrusiveListHook, T>)
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
		if constexpr (std::is_base_of_v<SafeLinkIntrusiveListHook, T>) {
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

	template<typename D>
	void clear_and_dispose(D &&disposer) noexcept {
		while (!empty()) {
			auto *item = &front();
			pop_front();
			disposer(item);
		}
	}

	template<typename P, typename D>
	void remove_and_dispose_if(P &&pred, D &&dispose) noexcept {
		auto *n = head.next;

		while (n != &head) {
			auto *i = Cast(n);
			n = n->next;

			if (pred(*i)) {
				ToHook(*i).unlink();
				--counter;
				dispose(i);
			}
		}
	}

	const T &front() const noexcept {
		return *Cast(head.next);
	}

	T &front() noexcept {
		return *Cast(head.next);
	}

	void pop_front() noexcept {
		ToHook(front()).unlink();
		--counter;
	}

	template<typename D>
	void pop_front_and_dispose(D &&disposer) noexcept {
		auto &i = front();
		ToHook(i).unlink();
		--counter;
		disposer(&i);
	}

	T &back() noexcept {
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
		using iterator_category = std::forward_iterator_tag;
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

		constexpr T &operator*() const noexcept {
			return *Cast(cursor);
		}

		constexpr T *operator->() const noexcept {
			return Cast(cursor);
		}

		iterator &operator++() noexcept {
			cursor = cursor->next;
			return *this;
		}
	};

	constexpr iterator begin() noexcept {
		return {head.next};
	}

	constexpr iterator end() noexcept {
		return {&head};
	}

	static constexpr iterator iterator_to(T &t) noexcept {
		return {&ToNode(t)};
	}

	class const_iterator final {
		friend IntrusiveList;

		const IntrusiveListNode *cursor;

		constexpr const_iterator(const IntrusiveListNode *_cursor) noexcept
			:cursor(_cursor) {}

	public:
		using iterator_category = std::forward_iterator_tag;
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

		constexpr const T &operator*() const noexcept {
			return *Cast(cursor);
		}

		constexpr const T *operator->() const noexcept {
			return Cast(cursor);
		}

		const_iterator &operator++() noexcept {
			cursor = cursor->next;
			return *this;
		}
	};

	constexpr const_iterator begin() const noexcept {
		return {head.next};
	}

	constexpr const_iterator end() const noexcept {
		return {&head};
	}

	static constexpr iterator iterator_to(const T &t) noexcept {
		return {&ToNode(t)};
	}

	iterator erase(iterator i) noexcept {
		auto result = std::next(i);
		ToHook(*i).unlink();
		--counter;
		return result;
	}

	template<typename D>
	iterator erase_and_dispose(iterator i, D &&disposer) noexcept {
		auto result = erase(i);
		disposer(&*i);
		return result;
	}

	void push_front(T &t) noexcept {
		insert(begin(), t);
	}

	void push_back(T &t) noexcept {
		insert(end(), t);
	}

	void insert(iterator p, T &t) noexcept {
		static_assert(!constant_time_size ||
			      !HookTraits::IsAutoUnlink(),
			      "Can't use auto-unlink hooks with constant_time_size");

		auto &existing_node = ToNode(*p);
		auto &new_node = ToNode(t);

		existing_node.prev->next = &new_node;
		new_node.prev = existing_node.prev;
		existing_node.prev = &new_node;
		new_node.next = &existing_node;

		++counter;
	}
};
