// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <wchar.h>

/**
 * Skips whitespace at the beginning of the string, and returns the
 * first non-whitespace character.  If the string has no
 * non-whitespace characters, then a pointer to the NULL terminator is
 * returned.
 */
[[gnu::pure]] [[gnu::returns_nonnull]] [[gnu::nonnull]]
const wchar_t *
StripLeft(const wchar_t *p) noexcept;

[[gnu::pure]] [[gnu::returns_nonnull]] [[gnu::nonnull]]
static inline wchar_t *
StripLeft(wchar_t *p) noexcept
{
	return const_cast<wchar_t *>(StripLeft((const wchar_t *)p));
}

/**
 * Skips whitespace at the beginning of the string, and returns the
 * first non-whitespace character or the end pointer.
 */
[[gnu::pure]] [[gnu::returns_nonnull]] [[gnu::nonnull]]
const wchar_t *
StripLeft(const wchar_t *p, const wchar_t *end) noexcept;

/**
 * Determine the string's end as if it was stripped on the right side.
 */
[[gnu::pure]] [[gnu::returns_nonnull]] [[gnu::nonnull]]
const wchar_t *
StripRight(const wchar_t *p, const wchar_t *end) noexcept;

/**
 * Determine the string's end as if it was stripped on the right side.
 */
[[gnu::pure]] [[gnu::returns_nonnull]] [[gnu::nonnull]]
static inline wchar_t *
StripRight(wchar_t *p, wchar_t *end) noexcept
{
	return const_cast<wchar_t *>(StripRight((const wchar_t *)p,
						(const wchar_t *)end));
}

/**
 * Determine the string's length as if it was stripped on the right
 * side.
 */
[[gnu::pure]] [[gnu::nonnull]]
size_t
StripRight(const wchar_t *p, size_t length) noexcept;

/**
 * Strip trailing whitespace by null-terminating the string.
 */
[[gnu::nonnull]]
void
StripRight(wchar_t *p) noexcept;

/**
 * Skip whitespace at the beginning and terminate the string after the
 * last non-whitespace character.
 */
[[gnu::returns_nonnull]] [[gnu::nonnull]]
wchar_t *
Strip(wchar_t *p) noexcept;
