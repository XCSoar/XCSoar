/*
 * Copyright (C) 2011-2016 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef WASCII_HXX
#define WASCII_HXX

#include "Compiler.h"

#include <stddef.h>
#include <wchar.h>

gcc_nonnull_all
void
CopyASCII(wchar_t *dest, const wchar_t *src);

gcc_nonnull_all
wchar_t *
CopyASCII(wchar_t *dest, size_t dest_size,
	  const wchar_t *src, const wchar_t *src_end);

gcc_nonnull_all
void
CopyASCII(wchar_t *dest, const char *src);

gcc_nonnull_all
wchar_t *
CopyASCII(wchar_t *dest, size_t dest_size, const char *src, const char *src_end);

gcc_nonnull_all
char *
CopyASCII(char *dest, size_t dest_size, const wchar_t *src, const wchar_t *src_end);

gcc_nonnull_all
void
CopyASCIIUpper(char *dest, const wchar_t *src);

#endif
