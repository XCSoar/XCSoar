/*
 * Copyright (C) 2013-2015 Max Kellermann <max.kellermann@gmail.com>
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

#include "StringCompare.hxx"
#include "StringAPI.hxx"

#include <string.h>

bool
StringStartsWith(const char *haystack, const char *needle)
{
	return memcmp(haystack, needle, StringLength(needle) * sizeof(needle[0])) == 0;
}

bool
StringEndsWith(const char *haystack, const char *needle)
{
	const size_t haystack_length = StringLength(haystack);
	const size_t needle_length = StringLength(needle);

	return haystack_length >= needle_length &&
		StringIsEqual(haystack + haystack_length - needle_length, needle);
}

bool
StringEndsWithIgnoreCase(const char *haystack, const char *needle)
{
	const size_t haystack_length = StringLength(haystack);
	const size_t needle_length = StringLength(needle);

	return haystack_length >= needle_length &&
		StringIsEqualIgnoreCase(haystack + haystack_length - needle_length,
					needle);
}

const char *
StringAfterPrefix(const char *string, const char *prefix)
{
	size_t prefix_length = strlen(prefix);
	return StringIsEqual(string, prefix, prefix_length)
		? string + prefix_length
		: nullptr;
}

const char *
StringAfterPrefixCI(const char *string, const char *prefix)
{
	size_t prefix_length = StringLength(prefix);
	return strncasecmp(string, prefix, prefix_length) == 0
		? string + prefix_length
		: nullptr;
}

bool
StringStartsWithIgnoreCase(const char *haystack, const char *needle)
{
	return StringIsEqualIgnoreCase(haystack, needle,
				       StringLength(needle) * sizeof(needle[0]));
}
