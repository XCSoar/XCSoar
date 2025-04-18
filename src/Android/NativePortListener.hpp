// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>

class PortListener;

namespace NativePortListener {

void Initialise(JNIEnv *env);
void Deinitialise(JNIEnv *env);

/**
 * Create a Java NativePortListener instance.  It is not bound to a
 * handler yet; call Set() to do this.
 */
jobject Create(JNIEnv *env, PortListener &listener);

} // namespace NativePortListener
