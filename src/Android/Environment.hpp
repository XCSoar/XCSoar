// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>
#include <cstddef>

class AllocatedPath;

namespace Environment {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

AllocatedPath
GetExternalStoragePublicDirectory(JNIEnv *env,
                                  const char *type) noexcept;

} // namespace Environment
