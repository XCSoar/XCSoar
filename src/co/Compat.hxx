// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <utility>

#if defined(_LIBCPP_VERSION) && defined(__clang__) && (__clang_major__ < 14 || (defined(__APPLE__) && __clang_major__ < 15) )
/* libc++ until 14 has the coroutine definitions in the
   std::experimental namespace */
/* the standard header is also missing in the Android NDK and on Apple
   Xcode, even though LLVM upstream has them */

#include <experimental/coroutine>

namespace std {
using std::experimental::coroutine_handle;
using std::experimental::suspend_never;
using std::experimental::suspend_always;
using std::experimental::noop_coroutine;
}

#else /* not clang */

#if defined __GNUC__ && defined _WIN32
#if __GNUC__ == 10
namespace std {
inline namespace __n4861 {
/* workaround for Windows linker error "multiple definition of ..."
   with GCC10: these two symbols are declared as "weak", but only
   adding "inline" and "selectany" makes the linker failure go away */
inline void __dummy_resume_destroy();
extern struct __noop_coro_frame __noop_coro_fr __attribute__((selectany));
}}
#endif
#endif

#include <coroutine>
#ifndef __cpp_impl_coroutine
#error Need -fcoroutines
#endif

#endif /* not clang */
