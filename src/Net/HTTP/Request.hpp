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

#ifndef NET_REQUEST_HPP
#define NET_REQUEST_HPP

#include "Easy.hxx"
#include "Slist.hxx"

#include <curl/curl.h>

#include <cstddef>
#include <cstdint>

namespace Net {
  class Session;
  class ResponseHandler;
  class MultiPartFormData;

  class Request {
#ifndef _WIN32
    static constexpr unsigned INFINITE = (unsigned)-1;
#endif

    Session &session;

    CurlEasy handle;

    CurlSlist request_headers;

    ResponseHandler &handler;

    /**
     * Was the response metadata already submitted to
     * ResponseHandler::ResponseReceived().
     */
    bool submitted = false;

  public:
    /**
     * Creates a Request that can be used to get data from a webserver.
     * @param session Session instance that is used for creating this Request
     * @param url the absolute URL of the request
     * @param timeout_ms Timeout used for creating this request
     */
    Request(Session &session, ResponseHandler &_handler,
            const char *url);

    ~Request();

    void AddHeader(const char *name, const char *value);

    void SetBasicAuth(const char *username, const char *password);

    /**
     * Change the method to POST and use the given request body.  It
     * must not be destructed until Send() returns.
     */
    void SetRequestBody(const MultiPartFormData &body);

    void SetVerifyPeer(bool value = true);

    /**
     * Send the request to the server and receive response headers.
     * This function fails if the connection could not be established
     * or if the response status is not successful.
     *
     * @param timeout_ms Timeout used for sending the request
     */
    void Send(unsigned timeout_ms=INFINITE);

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
     * EOF.
     */
    size_t Read(void *buffer, size_t buffer_size,
                unsigned timeout_ms=INFINITE);

  private:
    bool SubmitResponse() noexcept;

    size_t ResponseData(const uint8_t *ptr, size_t size);

    static size_t WriteCallback(char *ptr, size_t size, size_t nmemb,
                                void *userdata);
};
}

#endif
