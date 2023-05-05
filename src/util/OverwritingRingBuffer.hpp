// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cassert>

/**
 * A fixed-size ring buffer which deletes the oldest item when it
 * overflows.  It stores up to "size-1" items (for the full/empty
 * distinction).
 *
 * Not thread safe.
 */
template<class T, unsigned size>
class TrivialOverwritingRingBuffer
{
	friend class const_iterator;

public:
	class const_iterator {
		friend class TrivialOverwritingRingBuffer;

		const TrivialOverwritingRingBuffer &buffer;
		unsigned i;

		const_iterator(const TrivialOverwritingRingBuffer<T, size> &_buffer,
			       unsigned _i) noexcept
			:buffer(_buffer), i(_i)
		{
			assert(i < size);
		}

	public:
		const T &operator*() const noexcept {
			return buffer.data[i];
		}

		auto &operator++() noexcept {
			i = buffer.next(i);
			return *this;
		}

		auto &operator--() noexcept {
			i = buffer.previous(i);
			return *this;
		}

		bool operator==(const const_iterator &other) const noexcept {
			assert(&buffer == &other.buffer);
			return i == other.i;
		}

		bool operator!=(const const_iterator &other) const noexcept {
			assert(&buffer == &other.buffer);
			return i != other.i;
		}
	};

protected:
	T data[size];
	unsigned head, tail;

	constexpr
	TrivialOverwritingRingBuffer(unsigned _head, unsigned _tail)
		:head(_head), tail(_tail) {}

public:
	TrivialOverwritingRingBuffer() = default;

protected:
	static unsigned next(unsigned i) noexcept {
		assert(i < size);

		return (i + 1) % size;
	}

	static unsigned previous(unsigned i) noexcept {
		assert(i < size);
		if (i>0)
			return (i-1);
		else
			return size-1;
	}

public:
	bool empty() const noexcept {
		assert(head < size);
		assert(tail < size);

		return head == tail;
	}

	void clear() noexcept {
		head = tail = 0;
	}

	// returns last value added
	const T &last() const noexcept {
		assert(!empty());
		return data[previous(tail)];
	}

	const T &peek() const noexcept {
		assert(!empty());

		return data[head];
	}

	const T &shift() noexcept {
		/* this returns a reference to an item which is being
		   Invalidated - but that's okay, because it won't get
		   purged yet */
		const T &value = peek();
		head = next(head);
		return value;
	}

	void push(const T &value) {
		assert(tail < size);

		data[tail] = value;
		tail = next(tail);
		if (tail == head)
			/* the ring buffer is full - delete the oldest
			   item */
			head = next(head);
	}

	T pop() {
		assert(!empty());
		tail = previous(tail);
		return data[tail];
	}

	/**
	 * Returns a pointer to the oldest item.
	 */
	const_iterator begin() const noexcept {
		return const_iterator(*this, head);
	}

	/**
	 * Returns a pointer to end of the buffer (one item after the
	 * newest item).
	 */
	const_iterator end() const noexcept {
		return const_iterator(*this, tail);
	}

	static constexpr unsigned capacity() noexcept {
		return size;
	}
};

template<class T, unsigned size>
class OverwritingRingBuffer: public TrivialOverwritingRingBuffer<T, size>
{
public:
	OverwritingRingBuffer():TrivialOverwritingRingBuffer<T, size>(0, 0) {}
};
