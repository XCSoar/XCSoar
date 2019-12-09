/*
 * Copyright 2015-2021 Max Kellermann <max.kellermann@gmail.com>
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

#include "StringBuilder.hxx"
#include "StringAPI.hxx"

#ifdef _UNICODE
#include <tchar.h>
#endif

#include <stdarg.h>
#include <stdio.h>

template<typename T>
void
BasicStringBuilder<T>::Append(const_pointer src)
{
	Append(src, StringLength(src));
}

template<typename T>
void
BasicStringBuilder<T>::Append(const_pointer src, size_t length)
{
	CheckAppend(length);

	p = std::copy_n(src, length, p);
	*p = SENTINEL;
}

template class BasicStringBuilder<char>;

template<>
void
BasicStringBuilder<char>::Format(const_pointer fmt, ...)
{
	size_t size = GetRemainingSize();

	va_list ap;
	va_start(ap, fmt);
	size_t n = vsnprintf(GetTail(), size, fmt, ap);
	va_end(ap);

	if (n >= size - 1)
		throw Overflow();

	Extend(n);
}

#ifdef _UNICODE
template class BasicStringBuilder<TCHAR>;
#endif
