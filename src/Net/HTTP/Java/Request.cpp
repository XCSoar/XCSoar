/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "../Request.hpp"
#include "Version.hpp"
#include "Java/Global.hxx"
#include "Java/String.hxx"
#include "Java/InputStream.hxx"
#include "Java/URL.hxx"
#include "Java/Exception.hxx"
#include "Util/ConstBuffer.hxx"
#include "Util/StringAPI.hxx"

#include <assert.h>
#include <stdio.h>

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned timeout_ms)
  :env(Java::GetEnv())
{
  connection = Java::URL::openConnection(env, url);
  if (Java::DiscardException(env)) {
    connection = nullptr;
    input_stream = nullptr;
    return;
  }

  Java::URLConnection::setConnectTimeout(env, connection, (jint)timeout_ms);
  Java::URLConnection::setReadTimeout(env, connection, (jint)timeout_ms);
}

Net::Request::~Request()
{
  if (connection != nullptr)
    env->DeleteLocalRef(connection);

  if (input_stream != nullptr) {
    Java::InputStream::close(env, input_stream);
    env->ExceptionClear();

    env->DeleteLocalRef(input_stream);
  }
}

void
Net::Request::AddHeader(const char *name, const char *value)
{
  if (connection == nullptr)
    return;

  Java::String j_name(env, name);
  Java::String j_value(env, value);

  Java::URLConnection::addRequestProperty(env, connection, j_name, j_value);
}

static char *
Base64(char *dest, uint8_t a, uint8_t b, uint8_t c)
{
  static constexpr char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  *dest++ = base64[a >> 2];
  *dest++ = base64[((a & 0x03) << 4) | (b >> 4)];
  *dest++ = base64[((b & 0x0f) << 2) | (c >> 6)];
  *dest++ = base64[c & 0x3f];
  return dest;
}

static char *
Base64(char *dest, ConstBuffer<uint8_t> src)
{
  while (src.size >= 3) {
    dest = Base64(dest, src[0], src[1], src[2]);
    src.skip_front(3);
  }

  if (src.size == 2) {
    dest = Base64(dest, src[0], src[1], 0);
    dest[-1] = '=';
  } else if (src.size == 1) {
    dest = Base64(dest, src[0], 0, 0);
    dest[-2] = '=';
    dest[-1] = '=';
  }

  return dest;
}

void
Net::Request::SetBasicAuth(const char *username, const char *password)
{
  if (connection == nullptr)
    return;

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s:%s", username, password);

  char value[512];
  char *p = UnsafeCopyStringP(value, "Basic ");
  p = Base64(p, {(const uint8_t *)buffer, strlen(buffer)});
  *p = 0;

  AddHeader("Authorization", value);
}

bool
Net::Request::Send(unsigned _timeout_ms)
{
  if (connection == nullptr)
    return false;

  input_stream = Java::URLConnection::getInputStream(env, connection);
  if (Java::DiscardException(env)) {
    env->DeleteLocalRef(connection);
    connection = nullptr;
    input_stream = nullptr;
  }

  return input_stream != nullptr;
}

int64_t
Net::Request::GetLength() const
{
  assert(connection != nullptr);
  assert(input_stream != nullptr);

  return Java::URLConnection::getContentLength(env, connection);
}

ssize_t
Net::Request::Read(void *buffer, size_t buffer_size, unsigned timeout_ms)
{
  assert(connection != nullptr);
  assert(input_stream != nullptr);

  Java::URLConnection::setReadTimeout(env, connection, (jint)timeout_ms);

  Java::LocalRef<jbyteArray> array(env,
                                   (jbyteArray)env->NewByteArray(buffer_size));
  jint nbytes = Java::InputStream::read(env, input_stream, array.Get());
  if (Java::DiscardException(env))
    return -1;

  if (nbytes <= 0)
    return 0;

  env->GetByteArrayRegion(array.Get(), 0, nbytes, (jbyte *)buffer);
  return nbytes;
}
