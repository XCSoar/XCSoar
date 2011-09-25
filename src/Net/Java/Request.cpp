/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include <assert.h>

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned long timeout)
  :env(Java::GetEnv())
{
  Java::Class url_class(env, "java/net/URL");
  jmethodID url_ctor = env->GetMethodID(url_class.get(), "<init>",
                                        "(Ljava/lang/String;)V");
  assert(url_ctor != NULL);

  Java::String j_url(env, url);
  jobject url_object = env->NewObject(url_class.get(), url_ctor, j_url.get());
  if (env->ExceptionOccurred() || url_object == NULL) {
    env->ExceptionClear();
    input_stream = NULL;
    return;
  }

  jmethodID url_open = env->GetMethodID(url_class.get(), "openConnection",
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

  set_timeout_method = env->GetMethodID(connection_class.get(),
                                        "setConnectTimeout", "(I)V");
  assert(set_timeout_method != NULL);
  env->CallVoidMethod(connection, set_timeout_method, (jint)timeout);

  set_timeout_method = env->GetMethodID(connection_class.get(),
                                        "setReadTimeout", "(I)V");
  assert(set_timeout_method != NULL);

  jmethodID get_input_stream = env->GetMethodID(connection_class.get(),
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

  Java::Class stream_class(env, env->GetObjectClass(input_stream));
  read_method = env->GetMethodID(stream_class.get(), "read", "([B)I");
  assert(read_method != NULL);
  close_method = env->GetMethodID(stream_class.get(), "close", "()V");
  assert(close_method != NULL);
}

Net::Request::~Request()
{
  if (connection != NULL)
    env->DeleteLocalRef(connection);

  if (input_stream != NULL) {
    env->CallVoidMethod(input_stream, close_method);
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
  jint nbytes = env->CallIntMethod(input_stream, read_method, array.get());
  if (nbytes <= 0)
    return 0;

  env->GetByteArrayRegion(array.get(), 0, nbytes, (jbyte *)buffer);
  return nbytes;
}
