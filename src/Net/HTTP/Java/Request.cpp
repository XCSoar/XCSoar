/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Util/Base64.hxx"
#include "Util/StringAPI.hxx"

#include <assert.h>
#include <stdio.h>

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned timeout_ms)
  :env(Java::GetEnv())
{
  connection = Java::URL::openConnection(env, url);
  Java::RethrowException(env);

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
  assert(connection != nullptr);

  Java::String j_name(env, name);
  Java::String j_value(env, value);

  Java::URLConnection::addRequestProperty(env, connection, j_name, j_value);
}

void
Net::Request::SetBasicAuth(const char *username, const char *password)
{
  assert(connection != nullptr);

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s:%s", username, password);

  char value[512];
  char *p = UnsafeCopyStringP(value, "Basic ");
  p = Base64(p, {(const uint8_t *)buffer, strlen(buffer)});
  *p = 0;

  AddHeader("Authorization", value);
}

void
Net::Request::Send(unsigned _timeout_ms)
{
  assert(connection != nullptr);

  input_stream = Java::URLConnection::getInputStream(env, connection);
  Java::RethrowException(env);
}

int64_t
Net::Request::GetLength() const
{
  assert(connection != nullptr);
  assert(input_stream != nullptr);

  return Java::URLConnection::getContentLength(env, connection);
}

size_t
Net::Request::Read(void *buffer, size_t buffer_size, unsigned timeout_ms)
{
  assert(connection != nullptr);
  assert(input_stream != nullptr);

  Java::URLConnection::setReadTimeout(env, connection, (jint)timeout_ms);

  Java::LocalRef<jbyteArray> array(env,
                                   (jbyteArray)env->NewByteArray(buffer_size));
  jint nbytes = Java::InputStream::read(env, input_stream, array.Get());
  Java::RethrowException(env);

  if (nbytes <= 0)
    return 0;

  env->GetByteArrayRegion(array.Get(), 0, nbytes, (jbyte *)buffer);
  return nbytes;
}
