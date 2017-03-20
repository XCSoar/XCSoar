/*
 * Copyright (C) 2010-2015 Max Kellermann <max.kellermann@gmail.com>
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

#include <array>
#include <initializer_list>
#include <algorithm>

#include <assert.h>
#include <stddef.h>

/**
 * An array with a maximum size known at compile time.  It keeps track
 * of the actual length at runtime. The clear() function needs to be called
 * to initialize the class properly.
 */
template<class T, size_t max>
class TrivialArray {
	typedef std::array<T, max> Array;

public:
	typedef typename Array::size_type size_type;
	typedef T value_type;
	typedef typename Array::iterator iterator;
	typedef typename Array::const_iterator const_iterator;

protected:
	size_type the_size;
	Array data;

	constexpr
	TrivialArray(size_type _size):the_size(_size) {}

public:
	/**
	 * Non-initialising constructor.
	 */
	TrivialArray() = default;

	TrivialArray(size_type _size, const T &value):the_size(_size) {
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
	TrivialArray(std::initializer_list<U> l):the_size(l.size()) {
		std::move(l.begin(), l.end(), data.begin());
	}

	constexpr
	size_type capacity() const { return max; }

	constexpr
	size_type max_size() const {
		return max;
	}

	/**
	 * Forcibly set the specified size, without initialising or freeing
	 * new/excess elements.
	 */
	void resize(size_type new_size) {
		assert(new_size <= max_size());

		the_size = new_size;
	}

	/**
	 * Returns the number of allocated elements.
	 */
	constexpr
	size_type size() const {
		return the_size;
	}

	void shrink(size_type _size) {
		assert(_size <= the_size);

		the_size = _size;
	}

	constexpr
	bool empty() const {
		return the_size == 0;
	}

	constexpr
	bool full() const {
		return the_size == max;
	}

	/**
	 * Empties this array, but does not destruct its elements.
	 */
	void clear() {
		the_size = 0;
	}

	/**
	 * Returns one element.  No bounds checking.
	 */
	T &operator[](size_type i) {
		assert(i < size());

		return data[i];
	}

	/**
	 * Returns one constant element.  No bounds checking.
	 */
	const T &operator[](size_type i) const {
		assert(i < size());

		return data[i];
	}

	iterator begin() {
		return data.begin();
	}

	const_iterator begin() const {
		return data.begin();
	}

	iterator end() {
		return std::next(data.begin(), the_size);
	}

	const_iterator end() const {
		return std::next(data.begin(), the_size);
	}

	T &back() {
		assert(the_size > 0);

		return data[the_size - 1];
	}

	const T &back() const {
		assert(the_size > 0);

		return data[the_size - 1];
	}

	bool contains(const T &value) const {
		return std::find(begin(), end(), value) != end();
	}

	/**
	 * Return address of start of data segment.
	 */
	const T* raw() const {
		return &data[0];
	}

	/**
	 * Append an element at the end of the array, increasing the length
	 * by one.  No bounds checking.
	 */
	void append(const T &value) {
		assert(!full());

		data[the_size++] = value;
	}

	/**
	 * Increase the length by one and return a pointer to the new
	 * element, to be modified by the caller.  No bounds checking.
	 */
	T &append() {
		assert(!full());

		return data[the_size++];
	}

	/**
	 * Like append(), but checks if the array is already full (returns
	 * false in this case).
	 */
	bool checked_append(const T &value) {
		if (full())
			return false;

		append(value);
		return true;
	}

	/**
	 * Remove the item at the given index.
	 */
	void remove(size_type i) {
		assert(i < size());

		std::move(std::next(data.begin(), i + 1),
			  std::next(data.begin(), size()),
			  std::next(data.begin(), i));

		--the_size;
	}

	/**
	 * Remove an item by copying the last item over it.
	 */
	void quick_remove(size_type i) {
		assert(i < size());

		if (i < size() - 1)
			data[i] = std::move(data[size() - 1]);

		--the_size;
	}

	/* STL API emulation */

	void push_back(const T &value) {
		append(value);
	}

	T &front() {
		assert(the_size > 0);

		return data.front();
	}

	const T &front() const {
		assert(the_size > 0);

		return data.front();
	}
};

#endif
