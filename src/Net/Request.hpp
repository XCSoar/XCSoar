/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Net/CURL/Easy.hpp"

#include <curl/curl.h>
#include <stdint.h>
#endif

#ifdef HAVE_JAVA_NET
#include <jni.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace Net {
  class Session;

#ifdef HAVE_WININET
  class Connection;
#endif

  class Request {
#ifdef HAVE_WININET
    /** Internal connection handle */
    WinINet::HttpRequestHandle handle;

    /** Event handles that are triggered in certain situations */
    Trigger opened_event, completed_event;
    /** The last error code that was retrieved by the Callback() function */
    DWORD last_error;
#elif defined(ANDROID)
    static constexpr unsigned INFINITE = 0;
#else
    static constexpr unsigned INFINITE = (unsigned)-1;
#endif

#ifdef HAVE_CURL
    Session &session;

    CurlEasy handle;

    typedef FifoBuffer<uint8_t, CURL_MAX_WRITE_SIZE> Buffer;
    Buffer buffer;
#endif

#ifdef HAVE_JAVA_NET
    JNIEnv *env;

    jobject connection, input_stream;
#endif

  public:
    /**
     * Creates a Request that can be used to get data from a webserver.
     * @param session Session instance that is used for creating this Request
     * @param url the absolute URL of the request
     * @param timeout_ms Timeout used for creating this request
     */
    Request(Session &session, const char *url,
            unsigned timeout_ms=INFINITE);

#if defined(HAVE_CURL) || defined(HAVE_JAVA_NET)
    ~Request();
#endif

#ifdef HAVE_CURL
  protected:
    size_t ResponseData(const uint8_t *ptr, size_t size);

    static size_t WriteCallback(char *ptr, size_t size, size_t nmemb,
                                void *userdata);
#endif

  public:
    /**
     * Send the request to the server and receive response headers.
     * This function fails if the connection could not be established
     * or if the response status is not successful.
     *
     * @param timeout_ms Timeout used for sending the request
     * @return true on success
     */
    bool Send(unsigned timeout_ms=INFINITE);

    /**
     * Returns the total length of the response body.  Returns -1 if
     * the server did not announce a length, or if an error has
     * occurred.
     */
    int64_t GetLength() const;

    /**
     * Reads a number of bytes from the server.
     * This function must not be called before Send() !
     * @param timeout_ms Timeout used for retrieving the data chunk
     * @return Number of bytes that were read from the server. 0 means
     * EOF, -1 means error
     */
    ssize_t Read(void *buffer, size_t buffer_size,
                 unsigned timeout_ms=INFINITE);

#ifdef HAVE_WININET
    /** Internal callback function. Don't use this manually! */
    void Callback(DWORD status, LPVOID info, DWORD info_length);
#endif
  };
}

#endif
