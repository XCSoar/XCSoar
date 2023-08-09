// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <chrono>
#include <cmath>

/**
 * A specialization of std::chrono::duration which stores the duration
 * in seconds in a double-precision floating point variable.
 */
using FloatDuration = std::chrono::duration<double>;

[[gnu::const]]
static inline FloatDuration
fdim(FloatDuration a, FloatDuration b) noexcept
{
	return FloatDuration{std::fdim(a.count(), b.count())};
}
