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

#ifndef NET_REQUEST_HPP
#define NET_REQUEST_HPP

#include "Net/Features.hpp"

#ifdef HAVE_WININET
#include "Thread/Trigger.hpp"
#include "Net/WinINet/WinINet.hpp"
#endif

#ifdef HAVE_CURL
#include "Util/FifoBuffer.hpp"

#include <curl/curl.h>
#include <stdint.h>
#endif

#ifdef HAVE_JAVA_NET
#include <jni.h>
#endif

#include <tchar.h>

namespace Net {
  class Session;

#ifdef HAVE_WININET
  class Connection;
#endif

  class Request {
  public:
#ifdef HAVE_WININET
    /** Internal connection handle */
    WinINet::HttpRequestHandle handle;

    /** Event handles that are triggered in certain situations */
    Trigger opened_event, completed_event;
    /** The last error code that was retrieved by the Callback() function */
    DWORD last_error;
#elif defined(ANDROID)
    static const unsigned long INFINITE = 0;
#else
    static const unsigned long INFINITE = (unsigned long)-1;
#endif

#ifdef HAVE_CURL
    Session &session;

    CURL *handle;

    typedef FifoBuffer<uint8_t, CURL_MAX_WRITE_SIZE> Buffer;
    Buffer buffer;
#endif

#ifdef HAVE_JAVA_NET
    JNIEnv *env;

    jobject connection, input_stream;

    jmethodID set_timeout_method, read_method, close_method;
#endif

  public:
    /**
     * Creates a Request that can be used to get data from a webserver.
     * @param session Session instance that is used for creating this Request
     * @param url the absolute URL of the request
     * @param timeout Timeout used for creating this request
     */
    Request(Session &session, const TCHAR *url,
            unsigned long timeout = INFINITE);

#ifdef HAVE_WININET
    /**
     * Creates a Request that can be used to get data from a webserver.
     * @param connection Connection instance that is used for creating this Request
     * @param file The file to request (e.g. /downloads/index.htm)
     * @param timeout Timeout used for creating this request
     */
    Request(Connection &connection, const char *file,
            unsigned long timeout = INFINITE);
#endif

#if defined(HAVE_CURL) || defined(HAVE_JAVA_NET)
    ~Request();
#endif

#ifdef HAVE_CURL
  protected:
    size_t ResponseData(const uint8_t *ptr, size_t size);

    static size_t WriteCallback(void *ptr, size_t size, size_t nmemb,
                                void *userdata);
#endif

  public:
    /**
     * Returns whether the Request has been created successfully.
     * Note that this has nothing to do with a physical connection yet!
     */
    bool Created() const;

#ifdef HAVE_WININET
    /**
     * Send the request to the server. If this function fails the server
     * can't be reached. If the file doesn't exists the webserver usually
     * returns a valid 404 page.
     * @param timeout Timeout used for sending the request
     * @return True if the connection was established successfully and
     * the request was sent
     */
    bool Send(unsigned long timeout = INFINITE);
#endif

    /**
     * Reads a number of bytes from the server.
     * This function must not be called before Send() !
     * @param timeout Timeout used for retrieving the data chunk
     * @return Number of bytes that were read from the server. 0 means EOF.
     */
    size_t Read(void *buffer, size_t buffer_size, unsigned long timeout = INFINITE);

#ifdef HAVE_WININET
    /** Internal callback function. Don't use this manually! */
    void Callback(DWORD status, LPVOID info, DWORD info_length);
#endif
  };
}

#endif
