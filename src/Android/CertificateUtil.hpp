// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <jni.h>

class AllocatedPath;
class Context;

namespace CertificateUtil {

/**
 * Global initialisation.  Looks up the methods of the
 * CertificateUtil Java class.
 */
void Initialise(JNIEnv *env) noexcept;
void Deinitialise(JNIEnv *env) noexcept;

/**
 * Extract system CA certificates and return path to PEM file.
 * 
 * @param env JNI environment
 * @param context Android context for accessing cache directory
 * @return Path to PEM file, or nullptr if extraction failed
 */
AllocatedPath GetSystemCaCertificatesPath(JNIEnv *env, Context &context) noexcept;

} // namespace CertificateUtil

