// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

constexpr bool
HaveAddressSanitizer() noexcept
{
#ifdef __clang__
	/* clang provides __has_feature(address_sanitizer)
	   (https://clang.llvm.org/docs/AddressSanitizer.html#conditional-compilation-with-has-feature-address-sanitizer) */
	return __has_feature(address_sanitizer);

#else

	/* GCC defines __SANITIZE_ADDRESS__
	   (https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html) */
#ifdef __SANITIZE_ADDRESS__
	return true;
#else
	return false;
#endif

#endif
}
