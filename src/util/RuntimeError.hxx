// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <stdexcept> // IWYU pragma: export
#include <utility>

#include <stdio.h>

template<typename... Args>
static inline std::runtime_error
FormatRuntimeError(const char *fmt, Args&&... args) noexcept
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
	return std::runtime_error(buffer);
}

template<typename... Args>
inline std::invalid_argument
FormatInvalidArgument(const char *fmt, Args&&... args) noexcept
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
	return std::invalid_argument(buffer);
}
