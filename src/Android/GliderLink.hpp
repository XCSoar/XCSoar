// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

class Context;
class SensorListener;

namespace GliderLink {

void
Initialise(JNIEnv *env) noexcept;

void
Deinitialise(JNIEnv *env) noexcept;

Java::LocalObject
Create(JNIEnv *env, Context &context, SensorListener &listener);

} // namespace GliderLink
