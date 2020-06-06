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

#ifndef XCSOAR_WININET_HPP
#define XCSOAR_WININET_HPP

#include "Compiler.h"
#include "Util/NonCopyable.hpp"
#include "Thread/Trigger.hpp"

#include <wininet.h>
#include <tchar.h>
#include <assert.h>
#include <stdint.h>

namespace WinINet {
  class Handle : private NonCopyable {
    HINTERNET handle;

  public:
    Handle():handle(NULL) {}
    Handle(HINTERNET _handle):handle(_handle) {}

    ~Handle() {
      ::InternetCloseHandle(handle);
    }

  protected:
    HINTERNET Get() const {
      return handle;
    }

  public:
    bool IsDefined() const {
      return handle != NULL;
    }

    void Set(HINTERNET _handle) {
      assert(!IsDefined());

      handle = _handle;
    }

    void Clear() {
      ::InternetCloseHandle(handle);
      handle = NULL;
    }

    INTERNET_STATUS_CALLBACK SetStatusCallback(INTERNET_STATUS_CALLBACK lpfnInternetCallback) {
      assert(IsDefined());

      return ::InternetSetStatusCallback(Get(), lpfnInternetCallback);
    }
  };

  class SessionHandle : public Handle {
  public:
    SessionHandle() {}
    SessionHandle(HINTERNET handle):Handle(handle) {}

    HINTERNET Connect(LPCTSTR lpszServerName, INTERNET_PORT nServerPort,
                      LPCTSTR lpszUsername, LPCTSTR lpszPassword,
                      DWORD dwService, DWORD dwFlags,
                      DWORD_PTR dwContext) const
    {
      assert(IsDefined());

      return ::InternetConnect(Get(),
                               lpszServerName, nServerPort,
                               lpszUsername, lpszPassword,
                               dwService, dwFlags,
                               dwContext);
    }

#ifdef UNICODE
    HINTERNET Connect(LPCSTR lpszServerName, INTERNET_PORT nServerPort,
                      LPCSTR lpszUsername, LPCSTR lpszPassword,
                      DWORD dwService, DWORD dwFlags,
                      DWORD_PTR dwContext) const
    {
      assert(IsDefined());

      return ::InternetConnectA(Get(),
                                lpszServerName, nServerPort,
                                lpszUsername, lpszPassword,
                                dwService, dwFlags,
                                dwContext);
    }
#endif

    HINTERNET OpenUrl(LPCTSTR lpszUrl,
                      LPCTSTR lpszHeaders, DWORD dwHeadersLength,
                      DWORD dwFlags, DWORD_PTR dwContext) const
    {
      assert(IsDefined());

      return ::InternetOpenUrl(Get(),
                               lpszUrl, lpszHeaders, dwHeadersLength,
                               dwFlags, dwContext);
    }
  };

  class ConnectionHandle : public Handle {
  public:
    ConnectionHandle() {}
    ConnectionHandle(HINTERNET handle):Handle(handle) {}
  };

  class HttpConnectionHandle : public ConnectionHandle {
  public:
    HttpConnectionHandle() {}
    HttpConnectionHandle(HINTERNET handle):ConnectionHandle(handle) {}

    HINTERNET OpenRequest(LPCTSTR lpszVerb, LPCTSTR lpszObjectName,
                          LPCTSTR lpszVersion, LPCTSTR lpszReferer,
                          LPCTSTR *lplpszAcceptTypes,
                          DWORD dwFlags,
                          DWORD_PTR dwContext) const {
      assert(IsDefined());

      return ::HttpOpenRequest(Get(),
                               lpszVersion, lpszObjectName,
                               lpszVersion, lpszReferer,
                               lplpszAcceptTypes,
                               dwFlags,
                               dwContext);
    }

#ifdef UNICODE
    HINTERNET OpenRequest(LPCSTR lpszVerb, LPCSTR lpszObjectName,
                          LPCSTR lpszVersion, LPCSTR lpszReferer,
                          LPCSTR *lplpszAcceptTypes,
                          DWORD dwFlags,
                          DWORD_PTR dwContext) const {
      assert(IsDefined());

      return ::HttpOpenRequestA(Get(),
                                lpszVersion, lpszObjectName,
                                lpszVersion, lpszReferer,
                                lplpszAcceptTypes,
                                dwFlags,
                                dwContext);
    }
#endif
  };

  class RequestHandle : public Handle {
  public:
    RequestHandle() {}
    RequestHandle(HINTERNET handle):Handle(handle) {}

    bool Read(LPINTERNET_BUFFERS lpBuffersOut,
              DWORD dwFlags, DWORD_PTR dwContext) {
      assert(IsDefined());

      return ::InternetReadFileEx(Get(), lpBuffersOut, dwFlags, dwContext);
    }

#ifdef UNICODE
    bool Read(LPINTERNET_BUFFERSA lpBuffersOut,
              DWORD dwFlags, DWORD_PTR dwContext) {
      assert(IsDefined());

      return ::InternetReadFileExA(Get(), lpBuffersOut, dwFlags, dwContext);
    }
#endif
  };

  class HttpRequestHandle : public RequestHandle {
  public:
    HttpRequestHandle() {}
    HttpRequestHandle(HINTERNET handle):RequestHandle(handle) {}

    bool QueryInfo(DWORD dwInfoLevel,
                   LPVOID lpvBuffer, LPDWORD lpdwBufferLength,
                   LPDWORD lpdwIndex=NULL) const {
      assert(IsDefined());

      return ::HttpQueryInfo(Get(), dwInfoLevel, lpvBuffer, lpdwBufferLength,
                             lpdwIndex);
    }

    /**
     * Returns the HTTP status code or 0 if there was an error.
     */
    gcc_pure
    unsigned GetStatusCode() const {
      DWORD status;
      DWORD size = sizeof(status);

      return QueryInfo(HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                       &status, &size)
        ? (unsigned)status
        : 0;
    }

    gcc_pure
    int64_t GetContentLength() const {
      DWORD value;
      DWORD size = sizeof(value);

      return QueryInfo(HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER,
                       &value, &size)
        ? (int64_t)value
        : -1;
    }

    bool SendRequest(LPCTSTR lpszHeaders, DWORD dwHeadersLength,
                     LPVOID lpOptional, DWORD dwOptionalLength) {
      assert(IsDefined());

      return ::HttpSendRequest(Get(),
                               lpszHeaders, dwHeadersLength,
                               lpOptional, dwOptionalLength);
    }

    bool Read(LPINTERNET_BUFFERS lpBuffersOut,
              DWORD dwFlags, DWORD_PTR dwContext) {
      assert(IsDefined());

      return ::InternetReadFileEx(Get(), lpBuffersOut, dwFlags, dwContext);
    }

#ifdef UNICODE
    bool Read(LPINTERNET_BUFFERSA lpBuffersOut,
              DWORD dwFlags, DWORD_PTR dwContext) {
      assert(IsDefined());

      return ::InternetReadFileExA(Get(), lpBuffersOut, dwFlags, dwContext);
    }
#endif
  };
};

#endif

