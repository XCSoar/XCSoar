// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <fmt/core.h>
#if FMT_VERSION >= 80000 && FMT_VERSION < 90000
#include <fmt/format.h>
#endif

#include <exception>
#include <string_view>
#include <string>
#include <tuple>
#include <type_traits>

#ifdef _UNICODE
#include <wchar.h>
#include "util/ConvertString.hpp"
#endif

void
LogVFmt(fmt::string_view format_str, fmt::format_args args) noexcept;

#ifdef _UNICODE

// Helper to convert wchar_t* to UTF-8 string for logging
inline std::string
ConvertLogArg(const wchar_t* arg) noexcept
{
  if (arg == nullptr)
    return {};
  AllocatedString utf8 = ConvertWideToUTF8(arg);
  return utf8 != nullptr ? std::string(utf8.c_str()) : std::string();
}

inline std::string
ConvertLogArg(wchar_t* arg) noexcept
{
  return ConvertLogArg(static_cast<const wchar_t*>(arg));
}

// Convert and store arguments - converts wchar_t* to std::string, passes others through
template<typename T>
auto ConvertAndStore(T&& arg) noexcept
{
  if constexpr (std::is_same_v<std::decay_t<T>, const wchar_t*> ||
		std::is_same_v<std::decay_t<T>, wchar_t*>) {
    return ConvertLogArg(std::forward<T>(arg));
  } else {
    return std::forward<T>(arg);
  }
}

template<typename S, typename... Args>
void
LogFmt(const S &format_str, Args&&... args) noexcept
{
  // Convert wchar_t* arguments to UTF-8 strings and store them
  // Store all arguments (converted or original) in a tuple as lvalues
  auto converted = std::make_tuple(ConvertAndStore(std::forward<Args>(args))...);
  
  // Unpack the tuple and pass as lvalue references to fmt::make_format_args
#if FMT_VERSION >= 90000
  return LogVFmt(format_str,
		 std::apply([](auto&... converted_args) {
		     return fmt::make_format_args(converted_args...);
		   }, converted));
#else
  return LogVFmt(fmt::to_string_view(format_str),
		 std::apply([](auto&... converted_args) {
		     return fmt::make_args_checked<decltype(converted_args)...>(
		       format_str,
		       converted_args...);
		   }, converted));
#endif
}

#else

// On non-Windows, no conversion needed
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

#endif

/**
 * Write a line to the log file.
 *
 * @param s the line, which must not contain newline or carriage
 * return characters
 */
void
LogString(std::string_view s) noexcept;

/**
 * Write a formatted line to the log file.
 *
 * @param fmt the format string, which must not contain newline or
 * carriage return characters
 */
gcc_printf(1, 2)
void
LogFormat(const char *fmt, ...) noexcept;

#ifdef _UNICODE
void
LogFormat(const wchar_t *fmt, ...) noexcept;
#endif

#if !defined(NDEBUG)

#define LogDebug(...) LogFmt(__VA_ARGS__)

#else /* NDEBUG */

/* not using an empty inline function here because we don't want to
   evaluate the parameters */
#define LogDebug(...) do {} while (false)

#endif /* NDEBUG */

void
LogError(std::exception_ptr e) noexcept;

void
LogError(std::exception_ptr e, const char *msg) noexcept;
