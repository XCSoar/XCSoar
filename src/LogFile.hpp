// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LogFileDecl.hpp"
#include <fmt/core.h>
#if FMT_VERSION >= 80000 && FMT_VERSION < 90000
#include <fmt/format.h>
#endif

void
LogVFmt(fmt::string_view format_str, fmt::format_args args) noexcept;

template<typename S, typename... Args>
void
LogFmt(const S &format_str, Args&&... args) noexcept
{
#if FMT_VERSION >= 90000
	return LogVFmt(format_str,
		       fmt::make_format_args(args...));
#else
	return LogVFmt(fmt::to_string_view(format_str),
		       fmt::make_args_checked<Args...>(format_str,
						       args...));
#endif
}

#if !defined(NDEBUG)

#define LogDebug(...) LogFmt(__VA_ARGS__)

#else /* NDEBUG */

/* not using an empty inline function here because we don't want to
   evaluate the parameters */
#define LogDebug(...) do {} while (false)

#endif /* NDEBUG */
