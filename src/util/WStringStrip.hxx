/*
 * Copyright 2009-2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef WSTRING_STRIP_HXX
#define WSTRING_STRIP_HXX

/** \file */

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

#endif
