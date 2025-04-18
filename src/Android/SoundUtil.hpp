// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>

namespace SoundUtil {

void Initialise(JNIEnv *env);
void Deinitialise(JNIEnv *env);

bool Play(JNIEnv *env, jobject context, const char *name);
bool PlayExternal(JNIEnv *env, jobject context, const char *path);

} // namespace SoundUtil
