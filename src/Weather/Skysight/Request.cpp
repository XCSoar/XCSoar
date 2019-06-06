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

#include "Request.hpp"
#include "APIGlue.hpp"
#include "SkysightAPI.hpp"



#include "Thread/StandbyThread.hpp"

#include "OS/Path.hpp"
#include "OS/FileUtil.hpp"

#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/Request.hpp"
#include "Net/HTTP/Handler.hpp"
#include "Net/HTTP/FormData.hpp"

#include "IO/FileLineReader.hpp"

#include "LogFile.hpp"

#include "Util/StaticString.hxx"



void SkysightRequest::FileHandler::DataReceived(const void *data, size_t length) {

  size_t written = fwrite(data, 1, length, file);
  if (written != (size_t)length)
    throw SkysightRequestError();
  received += length;
}

void SkysightRequest::FileHandler::ResponseReceived(int64_t content_length) {
}

size_t SkysightRequest::BufferHandler::GetReceived() const {
  return received;
}
void SkysightRequest::BufferHandler::ResponseReceived(int64_t content_length) {
}
void SkysightRequest::BufferHandler::DataReceived(const void *data, size_t length) {
    size_t remaining = max_size - received;

    if (length > remaining)
      length = remaining;

    memcpy(buffer + received, data, length);
    received += length;

}

SkysightRequest::Status SkysightAsyncRequest::GetStatus() {
  std::lock_guard<Mutex> lock(mutex);
  Status s = status;
  return s;
}

SkysightCallType SkysightRequest::GetType() {
  return args.calltype;
}
void SkysightRequest::SetCredentials(const TCHAR *_key, const TCHAR *_username, 
                             const TCHAR *_password) {
  key = _key;
  if(_username != nullptr)
    username = _username;
  if(_password)
    password = _password;
}

SkysightCallType SkysightAsyncRequest::GetType() {
  std::lock_guard<Mutex> lock(mutex);
  SkysightCallType ct = args.calltype;
  return ct;
}
void SkysightAsyncRequest::SetCredentials(const TCHAR *_key, const TCHAR *_username, 
                             const TCHAR *_password)  {
  std::lock_guard<Mutex> lock(mutex);
  SkysightRequest::SetCredentials(_key, _username, _password);
}


void SkysightAsyncRequest::TriggerNullCallback(tstring &&ErrText) {
  if(args.cb) {
    args.cb(ErrText.c_str(), false, args.layer.c_str(), args.from);
  }
}


bool SkysightRequest::Process() {
  return RequestToFile();
}

tstring SkysightAsyncRequest::GetMessage() {
  std::lock_guard<Mutex> lock(mutex);
  tstring msg = tstring (_T("Downloading ")) + args.layer;
  return msg;
}

bool SkysightRequest::ProcessToString(tstring &response) {
 return RequestToBuffer(response);
}

void SkysightAsyncRequest::Process() {
  std::lock_guard<Mutex> lock(mutex);
  if (IsBusy()) return;
  Trigger();
}

void SkysightAsyncRequest::Tick() noexcept {
  status = Status::Busy;

  mutex.unlock();

  bool result;
  tstring resultStr;
  
  if(args.to_file) {
    result = RequestToFile();
    resultStr = args.path.c_str();
  } else {
    result = RequestToBuffer(resultStr);
  }
  
  if(result) {
    SkysightAPI::ParseResponse(resultStr.c_str(), result, args);
  } else {
    SkysightAPI::ParseResponse(_T("Could not fetch data from Skysight server."), result, args);
  }

  mutex.lock();
  status = (!result) ? Status::Error : Status::Complete;
}

void SkysightAsyncRequest::Done() {
    StandbyThread::LockStop();
}

bool SkysightRequest::RequestToFile() {

  LogFormat("Connecting to %s for %s with key %s", args.url.c_str(), args.path.c_str(), key.c_str());

  Path final_path = Path(args.path.c_str());
  AllocatedPath temp_path = final_path.WithExtension(".dltemp");

  if (!File::Delete(temp_path) && File::ExistsAny(temp_path))
    return  false;

  FILE *file = _tfopen(temp_path.c_str(), _T("wb"));
  if (file == nullptr)
    return false;

  bool success = true;
  Net::Session session;
  FileHandler handler(file);
  Net::Request request(session, handler, args.url.c_str());
  request.AddHeader("X-API-Key", key.c_str());

  tstring pBody;
  if(username.length() && password.length()) {
    request.AddHeader("Content-Type", "application/json");
    NarrowString<1024> creds;
    creds.Format("{\"username\":\"%s\",\"password\":\"%s\"}", username.c_str(), password.c_str());
    pBody = creds.c_str();
    request.SetRequestBody(pBody.c_str(), pBody.length());
    request.SetNoFailOnError();
  }
  
  request.SetVerifyPeer(false);
  
  try {
    request.Send(10000);
  } catch (const std::exception &exc) {
    success = false;
  }

  success &= fclose(file) == 0;

  if (!success) File::Delete(temp_path);

  file = NULL;

  if(success) {
    if (!File::Delete(final_path) && File::ExistsAny(final_path)) {
      File::Delete(temp_path);
      return false;
    }
    std::rename(temp_path.c_str(), args.path.c_str());
  }
  
  return success;
}

bool SkysightRequest::RequestToBuffer(tstring &response) {

  LogFormat("Connecting to %s with key %s", args.url.c_str(), key.c_str());

  bool success = true;
  Net::Session session;

  char buffer[10240];
  BufferHandler handler(buffer, sizeof(buffer));
  Net::Request request(session, handler, args.url.c_str());
  request.AddHeader("X-API-Key", key.c_str());

  tstring pBody;
  if(username.length() && password.length()) {
    request.AddHeader("Content-Type", "application/json");
    NarrowString<1024> creds;
    creds.Format("{\"username\":\"%s\",\"password\":\"%s\"}", username.c_str(), password.c_str());
    pBody = creds.c_str();
    request.SetRequestBody(pBody.c_str(), pBody.length());
    request.SetNoFailOnError();
  }
  
  request.SetVerifyPeer(false);

  try {
    request.Send(10000);
  } catch (const std::exception &exc) {
    success = false;
  }

  response = tstring(buffer, buffer + handler.GetReceived() / sizeof(buffer[0]));
  return success;
}

