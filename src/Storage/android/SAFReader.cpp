// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SAFReader.hpp"
#include "java/Global.hxx"
#include "java/InputStream.hxx"

#include <cassert>
#include <stdexcept>

SAFReader::SAFReader(JNIEnv *env, jobject is) noexcept
  :input_stream_(env, is)
{
  assert(is != nullptr);
}

SAFReader::~SAFReader() noexcept
{
  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return;

  Java::InputStream::close(env, input_stream_.Get());
  if (env->ExceptionCheck())
    env->ExceptionClear();
}

std::size_t
SAFReader::Read(std::span<std::byte> dest)
{
  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    throw std::runtime_error("SAFReader: no JNI env");

  const jsize len = static_cast<jsize>(dest.size());
  jbyteArray buf = env->NewByteArray(len);
  if (buf == nullptr)
    throw std::runtime_error("SAFReader: NewByteArray failed");

  jint n = Java::InputStream::read(env, input_stream_.Get(), buf);

  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    env->DeleteLocalRef(buf);
    throw std::runtime_error("SAFReader: read() threw");
  }

  if (n <= 0) {
    env->DeleteLocalRef(buf);
    return 0; // EOF
  }

  env->GetByteArrayRegion(buf, 0, n,
                          reinterpret_cast<jbyte *>(dest.data()));
  env->DeleteLocalRef(buf);
  return static_cast<std::size_t>(n);
}
