/*
 * Copyright 2010-2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef TRIVIAL_ARRAY_HXX
#define TRIVIAL_ARRAY_HXX

#include "ConstBuffer.hxx"
#include "WritableBuffer.hxx"

#include <array>
#include <initializer_list>
#include <algorithm>

#include <cassert>
#include <cstddef>

/**
 * An array with a maximum size known at compile time.  It keeps track
 * of the actual length at runtime. The clear() function needs to be
 * called to initialize the class properly.
 */
template<class T, std::size_t max>
class TrivialArray {
	using Array = std::array<T, max>;

public:
	using size_type = typename Array::size_type;
	using value_type = T;
	using iterator = typename Array::iterator;
	using const_iterator =  typename Array::const_iterator;
	using reference =  typename Array::reference;
	using const_reference =  typename Array::const_reference;
	using pointer =  typename Array::pointer;
	using const_pointer =  typename Array::const_pointer;

protected:
	size_type the_size;
	Array array;

	constexpr
	TrivialArray(size_type _size) noexcept:the_size(_size) {}

public:
	/**
	 * Non-initialising constructor.
	 */
	TrivialArray() = default;

	TrivialArray(size_type _size, const_reference value):the_size(_size) {
		std::fill(begin(), end(), value);
	}

	/**
	 * Initialise the array with values from the iterator range.
	 */
	template<typename I>
	TrivialArray(I _begin, I _end):the_size(0) {
		for (I i = _begin; i != _end; ++i)
			push_back(*i);
	}

	template<typename U>
	TrivialArray(std::initializer_list<U> init)
		:the_size(init.size()) {
		assert(init.size() <= max);

		std::move(init.begin(), init.end(), array.begin());
	}

	static constexpr size_type capacity() noexcept { return max; }

	constexpr
	size_type max_size() const noexcept {
		return max;
	}

	/**
	 * Forcibly set the specified size, without initialising or
	 * freeing new/excess elements.
	 */
	void resize(size_type new_size) noexcept {
		assert(new_size <= max_size());

		the_size = new_size;
	}

	/**
	 * Returns the number of allocated elements.
	 */
	constexpr
	size_type size() const noexcept {
		return the_size;
	}

	void shrink(size_type _size) noexcept {
		assert(_size <= the_size);

		the_size = _size;
	}

	constexpr
	bool empty() const noexcept {
		return the_size == 0;
	}

	constexpr
	bool full() const noexcept {
		return the_size == max;
	}

	/**
	 * Empties this array, but does not destruct its elements.
	 */
	void clear() noexcept {
		the_size = 0;
	}

	/**
	 * Returns one element.  No bounds checking.
	 */
	constexpr reference operator[](size_type i) noexcept {
		assert(i < size());

		return array[i];
	}

	/**
	 * Returns one constant element.  No bounds checking.
	 */
	constexpr const_reference operator[](size_type i) const noexcept {
		assert(i < size());

		return array[i];
	}

	constexpr iterator begin() noexcept {
		return array.begin();
	}

	constexpr const_iterator begin() const noexcept {
		return array.begin();
	}

	constexpr iterator end() noexcept {
		return std::next(array.begin(), the_size);
	}

	constexpr const_iterator end() const noexcept {
		return std::next(array.begin(), the_size);
	}

	constexpr reference back() noexcept {
		assert(the_size > 0);

		return array[the_size - 1];
	}

	constexpr const_reference back() const noexcept {
		assert(the_size > 0);

		return array[the_size - 1];
	}

	bool contains(const_reference value) const noexcept {
		return std::find(begin(), end(), value) != end();
	}

	/**
	 * Return address of start of data segment.
	 */
	constexpr pointer data() noexcept {
		return array.data();
	}

	constexpr const_pointer data() const noexcept {
		return array.data();
	}

	/**
	 * Append an element at the end of the array, increasing the
	 * length by one.  No bounds checking.
	 */
	void append(const_reference value) {
		assert(!full());

		array[the_size++] = value;
	}

	/**
	 * Increase the length by one and return a pointer to the new
	 * element, to be modified by the caller.  No bounds checking.
	 */
	reference append() noexcept {
		assert(!full());

		return array[the_size++];
	}

	/**
	 * Like append(), but checks if the array is already full
	 * (returns false in this case).
	 */
	bool checked_append(const_reference value) {
		if (full())
			return false;

		append(value);
		return true;
	}

	/**
	 * Remove the item at the given index.
	 */
	void remove(size_type i) noexcept {
		assert(i < size());

		std::move(std::next(array.begin(), i + 1),
			  std::next(array.begin(), size()),
			  std::next(array.begin(), i));

		--the_size;
	}

	/**
	 * Remove an item by copying the last item over it.
	 */
	void quick_remove(size_type i) noexcept {
		assert(i < size());

		if (i < size() - 1)
			array[i] = std::move(array[size() - 1]);

		--the_size;
	}

	template<typename I>
	void insert(size_type i, I _begin, I _end) {
		size_type n = std::distance(_begin, _end);
		assert(the_size + n < capacity());

		auto dest_begin = std::next(begin(), i);
		auto dest_end = end();
		the_size += n;

		std::move_backward(dest_begin, dest_end, end());
		std::copy(_begin, _end, dest_begin);
	}

	/* STL API emulation */

	void push_back(const_reference value) {
		append(value);
	}

	template<typename... Args>
	void emplace_back(Args&&... args) {
		append() = T(std::forward<Args>(args)...);
	}

	constexpr reference front() noexcept {
		assert(the_size > 0);

		return array.front();
	}

	constexpr const_reference front() const noexcept {
		assert(the_size > 0);

		return array.front();
	}

	constexpr operator WritableBuffer<T>() noexcept {
		return {data(), size()};
	}

	constexpr operator ConstBuffer<T>() const noexcept {
		return {data(), size()};
	}
};

#endif
