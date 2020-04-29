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
#ifndef WEATHER_SKYSIGHTREQUEST_HPP
#define WEATHER_SKYSIGHTREQUEST_HPP

#include "APIGlue.hpp"
#include "Thread/StandbyThread.hpp"
#include "Util/tstring.hpp"

#include "OS/Path.hpp"
#include "OS/FileUtil.hpp"

#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/Request.hpp"
#include "Net/HTTP/Handler.hpp"

#include "IO/FileLineReader.hpp"

#include "LogFile.hpp"


class SkysightRequestError {};

struct SkysightRequest {
  
public:
  enum class Status {Idle, Busy, Complete, Error};
  
  
  SkysightRequest(const SkysightRequestArgs _args) : args(_args) {};
  bool Process();  
  bool ProcessToString(tstring &response);
  
  SkysightCallType GetType();
  void SetCredentials(const TCHAR *_key, const TCHAR *_username = nullptr,
                      const TCHAR *_password = nullptr);
  
  class FileHandler final : public Net::ResponseHandler {
    FILE *file;
    size_t received = 0;

  public:
    FileHandler(FILE *_file) :file(_file) {};
    bool DataReceived(const void *data, size_t length) noexcept override;
    bool ResponseReceived(int64_t content_length) noexcept override;
  };

  class BufferHandler final : public Net::ResponseHandler {
    uint8_t *buffer;
    const size_t max_size;
    size_t received = 0;
    
  public:
    BufferHandler(void *_buffer, size_t _max_size)
      :buffer((uint8_t *)_buffer), max_size(_max_size) {}
    size_t GetReceived() const;
    bool ResponseReceived(int64_t content_length) noexcept override;
    bool DataReceived(const void *data, size_t length) noexcept override;
  };
  
protected:
  const SkysightRequestArgs args;
  tstring key, username, password;
  bool RequestToFile();
  bool RequestToBuffer(tstring &response);
};

struct SkysightAsyncRequest final : public SkysightRequest, public StandbyThread {
public:

  SkysightAsyncRequest(const SkysightRequestArgs _args) : SkysightRequest(_args), StandbyThread("SkysightAPIRequest"),
    status(Status::Idle) {};

  Status GetStatus();

  void Process();
  ~SkysightAsyncRequest() {
    StandbyThread::LockStop();
  }
  void Done();
  tstring GetMessage();
  
  SkysightCallType GetType();
  void SetCredentials(const TCHAR *_key, const TCHAR *_username = nullptr,
                      const TCHAR *_password = nullptr);
  

  void TriggerNullCallback(tstring &&ErrText);
  
  
  
  
protected:
  Status status;
  void Tick() noexcept override;
};

#endif
