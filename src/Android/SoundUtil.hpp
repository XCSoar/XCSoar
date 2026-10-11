// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>

#include <span>

namespace SoundUtil {

void Initialise(JNIEnv *env);
void Deinitialise(JNIEnv *env);

bool Play(JNIEnv *env, jobject context, const char *name);
bool PlayExternal(JNIEnv *env, jobject context, const char *path);

/**
 * Play the bundled sounds one after another, replacing a sequence
 * which is still playing.
 */
bool PlaySequence(JNIEnv *env, jobject context,
                  std::span<const char *const> names);

} // namespace SoundUtil
