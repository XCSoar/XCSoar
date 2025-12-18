// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CertificateUtil.hpp"
#include "Context.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "java/Path.hxx"
#include "system/FileUtil.hpp"
#include "LogFile.hpp"

namespace CertificateUtil {

static Java::TrivialClass cls;
static jmethodID extractSystemCertificates_method;

void
Initialise(JNIEnv *env) noexcept
{
  cls.Find(env, "org/xcsoar/CertificateUtil");

  extractSystemCertificates_method =
    env->GetStaticMethodID(cls, "extractSystemCertificates",
                          "(Landroid/content/Context;)Ljava/lang/String;");
  
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    extractSystemCertificates_method = nullptr;
  }
}

void
Deinitialise(JNIEnv *env) noexcept
{
  cls.Clear(env);
}

AllocatedPath
GetSystemCaCertificatesPath(JNIEnv *env, Context &context) noexcept
{
  assert(env != nullptr);
  assert(cls.IsDefined());

  jstring result = (jstring)env->CallStaticObjectMethod(
    cls, extractSystemCertificates_method, context.Get());

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    LogFormat("CertificateUtil: Java exception during certificate extraction");
    return nullptr;
  }

  if (result == nullptr) {
    LogFormat("CertificateUtil: Failed to extract system certificates");
    return nullptr;
  }

  Java::String path{env, result};

  AllocatedPath pem_path = Java::ToPathChecked(path);
  if (pem_path == nullptr) {
    LogFormat("CertificateUtil: Certificate path is null");
    return nullptr;
  }

  // Verify file exists
  if (!File::Exists(pem_path)) {
    LogFormat("CertificateUtil: PEM file does not exist: %s",
              pem_path.c_str());
    return nullptr;
  }

  return pem_path;
}

} // namespace CertificateUtil
