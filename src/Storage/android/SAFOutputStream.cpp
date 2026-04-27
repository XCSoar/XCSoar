// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SAFOutputStream.hpp"
#include "java/Global.hxx"

#include <cassert>
#include <stdexcept>

jmethodID SAFOutputStream::write_method_ = nullptr;
jmethodID SAFOutputStream::close_method_ = nullptr;
jmethodID SAFOutputStream::flush_method_ = nullptr;

void
SAFOutputStream::Initialise(JNIEnv *env) noexcept
{
  jclass cls = env->FindClass("java/io/OutputStream");
  if (cls == nullptr)
    return;

  write_method_ = env->GetMethodID(cls, "write", "([B)V");
  close_method_ = env->GetMethodID(cls, "close", "()V");
  flush_method_ = env->GetMethodID(cls, "flush", "()V");
  env->DeleteLocalRef(cls);
}

SAFOutputStream::SAFOutputStream(JNIEnv *env, jobject os) noexcept
  :output_stream_(env, os)
{
  assert(os != nullptr);
}

SAFOutputStream::~SAFOutputStream() noexcept
{
  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return;

  // Flush + close, ignoring errors in destructor.
  env->CallVoidMethod(output_stream_.Get(), flush_method_);
  if (env->ExceptionCheck())
    env->ExceptionClear();

  env->CallVoidMethod(output_stream_.Get(), close_method_);
  if (env->ExceptionCheck())
    env->ExceptionClear();
}

void
SAFOutputStream::Write(std::span<const std::byte> src)
{
  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    throw std::runtime_error("SAFOutputStream: no JNI env");

  const jsize len = static_cast<jsize>(src.size());
  jbyteArray buf = env->NewByteArray(len);
  if (buf == nullptr)
    throw std::runtime_error("SAFOutputStream: NewByteArray failed");

  env->SetByteArrayRegion(buf, 0, len,
                          reinterpret_cast<const jbyte *>(src.data()));

  env->CallVoidMethod(output_stream_.Get(), write_method_, buf);
  env->DeleteLocalRef(buf);

  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    throw std::runtime_error("SAFOutputStream: write() threw");
  }
}

void
SAFOutputStream::Commit()
{
  Close();
}

void
SAFOutputStream::Close()
{
  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return;

  env->CallVoidMethod(output_stream_.Get(), flush_method_);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    throw std::runtime_error("SAFOutputStream: flush() failed");
  }

  env->CallVoidMethod(output_stream_.Get(), close_method_);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    throw std::runtime_error("SAFOutputStream: close() failed");
  }
}
