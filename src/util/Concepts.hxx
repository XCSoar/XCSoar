// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <concepts>

/**
 * Compatibility wrapper for std::invocable which is unavailable in
 * Apple Xcode.
 */
template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

template<typename F, typename T>
concept Disposer = std::invocable<F, T *>;
