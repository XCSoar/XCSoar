// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "TrivialArray.hxx"

/**
 * An array with a maximum size known at compile time.  It keeps track
 * of the actual length at runtime.
 */
template<class T, std::size_t max>
class StaticArray: public TrivialArray<T, max> {
public:
	constexpr StaticArray():TrivialArray<T, max>(0) {}

	using TrivialArray<T, max>::TrivialArray;
};
