/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Net/Request.hpp"
#include "Version.hpp"
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Java/String.hpp"
#include "Java/InputStream.hpp"

#include <assert.h>

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned long timeout)
  :env(Java::GetEnv())
{
  Java::Class url_class(env, "java/net/URL");
  jmethodID url_ctor = env->GetMethodID(url_class.Get(), "<init>",
                                        "(Ljava/lang/String;)V");
  assert(url_ctor != NULL);

  Java::String j_url(env, url);
  jobject url_object = env->NewObject(url_class.Get(), url_ctor, j_url.Get());
  if (env->ExceptionOccurred() || url_object == NULL) {
    env->ExceptionClear();
    input_stream = NULL;
    return;
  }

  jmethodID url_open = env->GetMethodID(url_class.Get(), "openConnection",
                                        "()Ljava/net/URLConnection;");
  assert(url_open != NULL);

  connection = env->CallObjectMethod(url_object, url_open);
  env->DeleteLocalRef(url_object);
  if (env->ExceptionOccurred() || connection == NULL) {
    env->ExceptionClear();
    connection = NULL;
    input_stream = NULL;
    return;
  }

  Java::Class connection_class(env, env->GetObjectClass(connection));

  set_timeout_method = env->GetMethodID(connection_class.Get(),
                                        "setConnectTimeout", "(I)V");
  assert(set_timeout_method != NULL);
  env->CallVoidMethod(connection, set_timeout_method, (jint)timeout);

  set_timeout_method = env->GetMethodID(connection_class.Get(),
                                        "setReadTimeout", "(I)V");
  assert(set_timeout_method != NULL);

  jmethodID get_input_stream = env->GetMethodID(connection_class.Get(),
                                                "getInputStream",
                                                "()Ljava/io/InputStream;");
  assert(get_input_stream != NULL);

  input_stream = env->CallObjectMethod(connection, get_input_stream);
  if (env->ExceptionOccurred() || input_stream == NULL) {
    env->ExceptionClear();
    env->DeleteLocalRef(connection);
    connection = NULL;
    input_stream = NULL;
    return;
  }
}

Net::Request::~Request()
{
  if (connection != NULL)
    env->DeleteLocalRef(connection);

  if (input_stream != NULL) {
    Java::InputStream::close(env, input_stream);
    env->ExceptionClear();

    env->DeleteLocalRef(input_stream);
  }
}

bool
Net::Request::Created() const
{
  return input_stream != NULL;
}

size_t
Net::Request::Read(void *buffer, size_t buffer_size, unsigned long timeout)
{
  assert(connection != NULL);
  assert(input_stream != NULL);

  env->CallVoidMethod(connection, set_timeout_method, (jint)timeout);

  Java::LocalRef<jbyteArray> array(env,
                                   (jbyteArray)env->NewByteArray(buffer_size));
  jint nbytes = Java::InputStream::read(env, input_stream, array.Get());
  if (nbytes <= 0)
    return 0;

  env->GetByteArrayRegion(array.Get(), 0, nbytes, (jbyte *)buffer);
  return nbytes;
}
