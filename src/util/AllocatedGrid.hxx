/*
 * Copyright (C) 2010 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef ALLOCATED_GRID_HXX
#define ALLOCATED_GRID_HXX

#include "AllocatedArray.hxx"

#include <cassert>

/**
 * A two dimensional array allocated on the heap with a length
 * determined at runtime.
 */
template<class T>
class AllocatedGrid {
protected:
	AllocatedArray<T> array;
	unsigned width = 0, height = 0;

public:
	using size_type = typename AllocatedArray<T>::size_type;
	using reference = typename AllocatedArray<T>::reference;
	using const_reference = typename AllocatedArray<T>::const_reference;
	using iterator = typename AllocatedArray<T>::iterator;
	using const_iterator = typename AllocatedArray<T>::const_iterator;

	constexpr AllocatedGrid() noexcept = default;
	AllocatedGrid(unsigned _width, unsigned _height) noexcept
		:array(_width * _height), width(_width), height(_height) {}

	constexpr bool IsDefined() const noexcept {
		return array.size() > 0;
	}

	constexpr unsigned GetWidth() const noexcept {
		return width;
	}

	constexpr unsigned GetHeight() const noexcept {
		return height;
	}

	constexpr size_type GetSize() const noexcept {
		return size_type(width) * size_type(height);
	}

	const_reference Get(unsigned x, unsigned y) const noexcept {
		assert(x < width);
		assert(y < height);

		return array[y * width + x];
	}

	reference Get(unsigned x, unsigned y) noexcept {
		assert(x < width);
		assert(y < height);

		return array[y * width + x];
	}

	const_reference GetLinear(size_type i) const noexcept {
		assert(i < GetSize());

		return array[i];
	}

	reference GetLinear(size_type i) noexcept {
		assert(i < GetSize());

		return array[i];
	}

	iterator begin() noexcept {
		return array.begin();
	}

	constexpr const_iterator begin() const noexcept {
		return array.begin();
	}

	iterator end() noexcept {
		return begin() + GetSize();
	}

	constexpr const_iterator end() const noexcept {
		return begin() + GetSize();
	}

	iterator GetPointerAt(unsigned x, unsigned y) noexcept {
		assert(x < width);
		assert(y < height);

		return begin() + y * width + x;
	}

	const_iterator GetPointerAt(unsigned x, unsigned y) const noexcept {
		assert(x < width);
		assert(y < height);

		return begin() + y * width + x;
	}

	void Reset() noexcept {
		width = height = 0;
		array.ResizeDiscard(0);
	}

	void GrowDiscard(unsigned _width, unsigned _height) noexcept {
		array.GrowDiscard(_width * _height);
		width = _width;
		height = _height;
	}

	/**
	 * Resize the grid, preserving as many old values as fit into the
	 * new dimensions, and fill newly allocated array slots.
	 */
	void GrowPreserveFill(unsigned _width, unsigned _height,
			      const_reference fill=T()) noexcept {
		if (_width < width) {
			const unsigned h = std::min(height, _height);
			const auto end = array.begin() + h * width;

			for (auto in = array.begin() + width, out = array.begin() + _width;
			     in < end; in += width) {
				out = std::move(in, in + _width, out);
			}

			width = _width;
		}

		const size_type h = std::min(height, _height);
		const size_type fill_start = h > 0
			? (h - 1) * _width + width
			: 0;

		array.GrowPreserve(_width * _height, width * height);

		if (_width > width) {
			const unsigned delta_w = _width - width;
			const auto end = array.begin();

			for (auto in = array.begin() + (h - 1) * width,
				     out = array.begin() + (h - 1) * _width + width;
			     in > end; in -= width, out -= delta_w) {
				out = std::move_backward(in, in + width, out);
				std::fill(out - delta_w, out, fill);
			}

			width = _width;
		}

		height = _height;

		size_type new_size = GetSize();
		if (fill_start < new_size)
			std::fill(begin() + fill_start, begin() + new_size, fill);
	}
};

#endif
