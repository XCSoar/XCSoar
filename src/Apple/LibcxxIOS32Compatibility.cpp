// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <limits>
#include <mutex>
#include <pthread.h>

#import <UIKit/UIKit.h>

/*
 * Work around deployment to 32-bit iOS 6 devices while building with a modern
 * Apple toolchain and a Theos legacy SDK.  The resulting binary can reference
 * libc++ ABI entry points that are declared by the build headers but missing
 * from the old libc++ dylib available on armv7 iOS.
 *
 * These definitions were verified against Theos iPhoneOS9.3.sdk and
 * iPhoneOS10.3.sdk for an armv7 deployment target.  They intentionally mirror
 * private libc++ ABI symbols in namespace std::__1; if the selected SDK,
 * libc++ headers, or target runtime changes, re-check the mangled symbols and
 * function signatures instead of assuming this shim is still compatible.
 */
namespace std {
inline namespace __1 {

time_t
chrono::system_clock::to_time_t(const time_point &t) noexcept
{
  return chrono::duration_cast<chrono::seconds>(t.time_since_epoch()).count();
}

void
condition_variable::__do_timed_wait(unique_lock<mutex> &lock,
                                    chrono::time_point<chrono::system_clock,
                                                       chrono::nanoseconds> tp) noexcept
{
  if (!lock.owns_lock())
    return;

  const auto d = tp.time_since_epoch();
  const auto s = chrono::duration_cast<chrono::seconds>(d);

  timespec ts;
  if (s.count() < std::numeric_limits<decltype(ts.tv_sec)>::max()) {
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(s.count());
    ts.tv_nsec = (d - s).count();
  } else {
    ts.tv_sec = std::numeric_limits<decltype(ts.tv_sec)>::max();
    ts.tv_nsec = 999999999;
  }

#ifndef NDEBUG
  const int result =
#endif
    pthread_cond_timedwait(native_handle(), lock.mutex()->native_handle(), &ts);
#ifndef NDEBUG
  assert(result == 0 || result == ETIMEDOUT);
#endif
}

} // inline namespace __1
} // namespace std

extern "C" {

struct dyld_build_version_t {
  uint32_t platform;
  uint32_t version;
};

int32_t
availability_version_check(uint32_t count,
                           const dyld_build_version_t versions[]) noexcept
{
  unsigned major = 0, minor = 0, patch = 0;
  sscanf(UIDevice.currentDevice.systemVersion.UTF8String, "%u.%u.%u",
         &major, &minor, &patch);

  const uint32_t ios_version = (major << 16) | (minor << 8) | patch;

  for (uint32_t i = 0; i < count; ++i) {
    if (versions[i].platform == 2)
      return ios_version >= versions[i].version;
  }

  return 0;
}

int32_t
_availability_version_check(uint32_t count,
                            const dyld_build_version_t versions[]) noexcept
{
  return availability_version_check(count, versions);
}

} // extern "C"
