// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Request.hpp"
#include "APIGlue.hpp"
#include "SkysightAPI.hpp"
#include "thread/StandbyThread.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "net/http/Init.hpp"
#include "lib/curl/Request.hxx"
#include "lib/curl/Handler.hxx"
#include "io/FileLineReader.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"
#include "lib/curl/Slist.hxx"

void
SkysightRequest::FileHandler::OnData(std::span<const std::byte> data)
{
  size_t written = fwrite(data.data(), 1, data.size(), file);
  if (written != (size_t)data.size())
    throw SkysightRequestError();

  received += data.size();
}

void
SkysightRequest::FileHandler::OnHeaders(unsigned status,
    __attribute__((unused)) Curl::Headers &&headers) {
  LogFormat("FileHandler::OnHeaders %d", status);
}

void
SkysightRequest::FileHandler::OnEnd() {
  const std::lock_guard<Mutex> lock(mutex);
  done = true;
  cond.notify_one();
}

void
SkysightRequest::FileHandler::OnError(std::exception_ptr e) noexcept {
  LogFormat("FileHandler::OnError");
  LogError(e);
  const std::lock_guard<Mutex> lock(mutex);
  error = std::move(e);
  done = true;
  cond.notify_one();
}

void
SkysightRequest::FileHandler::Wait() {
  std::unique_lock<Mutex> lock(mutex);
  cond.wait(lock, [this]{ return done; });

  if (error)
    std::rethrow_exception(error);
}

size_t
SkysightRequest::BufferHandler::GetReceived() const
{
  return received;
}

void
SkysightRequest::BufferHandler::OnData(std::span<const std::byte> data)
{
  memcpy(buffer + received, data.data(), data.size());
  received += data.size();
}

void
SkysightRequest::BufferHandler::OnHeaders(unsigned status,
    __attribute__((unused)) Curl::Headers &&headers) {
  LogFormat("BufferHandler::OnHeaders %d", status);
}

void
SkysightRequest::BufferHandler::OnEnd() {
  const std::lock_guard<Mutex> lock(mutex);
  done = true;
  cond.notify_one();
}

void
SkysightRequest::BufferHandler::OnError(std::exception_ptr e) noexcept {
  LogFormat("BufferHandler::OnError");
  LogError(e);
  const std::lock_guard<Mutex> lock(mutex);
  error = std::move(e);
  done = true;
  cond.notify_one();
}

void
SkysightRequest::BufferHandler::Wait() {
  std::unique_lock<Mutex> lock(mutex);
  cond.wait(lock, [this]{ return done; });

  if (error)
    std::rethrow_exception(error);
}

SkysightRequest::Status
SkysightAsyncRequest::GetStatus()
{
  std::lock_guard<Mutex> lock(mutex);
  Status s = status;
  return s;
}

SkysightCallType
SkysightRequest::GetType()
{
  return args.calltype;
}

void
SkysightRequest::SetCredentials(const TCHAR *_key, const TCHAR *_username, 
				const TCHAR *_password)
{
  key = _key;
  if(_username != nullptr)
    username = _username;
  if(_password)
    password = _password;
}

SkysightCallType
SkysightAsyncRequest::GetType()
{
  std::lock_guard<Mutex> lock(mutex);
  SkysightCallType ct = args.calltype;
  return ct;
}

void
SkysightAsyncRequest::SetCredentials(const TCHAR *_key,
				     const TCHAR *_username,
				     const TCHAR *_password)
{
  std::lock_guard<Mutex> lock(mutex);
  SkysightRequest::SetCredentials(_key, _username, _password);
}

void
SkysightAsyncRequest::TriggerNullCallback(tstring &&ErrText)
{
  if(args.cb)
    args.cb(ErrText.c_str(), false, args.layer.c_str(), args.from);
}

bool
SkysightRequest::Process()
{
  return RequestToFile();
}

tstring
SkysightAsyncRequest::GetMessage()
{
  std::lock_guard<Mutex> lock(mutex);
  tstring msg = tstring (_T("Downloading ")) + args.layer;
  return msg;
}

bool
SkysightRequest::ProcessToString(tstring &response)
{
  return RequestToBuffer(response);
}

void
SkysightAsyncRequest::Process()
{
  std::lock_guard<Mutex> lock(mutex);
  if (IsBusy()) return;
  Trigger();
}

void
SkysightAsyncRequest::Tick() noexcept
{
  status = Status::Busy;

  mutex.unlock();

  bool result;
  tstring resultStr;

  if (args.to_file) {
    result = RequestToFile();
    resultStr = args.path.c_str();
  } else {
    result = RequestToBuffer(resultStr);
  }

  if (result) {
    SkysightAPI::ParseResponse(resultStr.c_str(), result, args);
  } else {
    SkysightAPI::ParseResponse(_T("Could not fetch data from Skysight server."),
			       result, args);
  }

  mutex.lock();
  status = (!result) ? Status::Error : Status::Complete;
}

void
SkysightAsyncRequest::Done()
{
  StandbyThread::LockStop();
}

bool
SkysightRequest::RequestToFile()
{
  LogFormat("Connecting to %s for %s with key %s", args.url.c_str(), args.path.c_str(), key.c_str());

  Path final_path = Path(args.path.c_str());
  AllocatedPath temp_path = final_path.WithSuffix(".dltemp");

  if (!File::Delete(temp_path) && File::ExistsAny(temp_path))
    return  false;

  FILE *file = _tfopen(temp_path.c_str(), _T("wb"));
  if (file == nullptr)
    return false;

  bool success = true;
  FileHandler handler(file);
  CurlRequest request(*Net::curl, args.url.c_str(), handler);
  CurlSlist request_headers;
  char api_key_buffer[4096];
  snprintf(api_key_buffer, sizeof(api_key_buffer), "%s: %s", "X-API-Key", key.c_str());
  request_headers.Append(api_key_buffer);

  tstring pBody;
  if (username.length() && password.length()) {
    char content_type_buffer[4096];
    snprintf(content_type_buffer, sizeof(content_type_buffer), "%s: %s", "Content-Type", "application/json");
    request_headers.Append(content_type_buffer);
    NarrowString<1024> creds;
    creds.Format("{\"username\":\"%s\",\"password\":\"%s\"}", username.c_str(), password.c_str());
    pBody = creds.c_str();
    request.SetRequestBody(pBody.c_str(), pBody.length());
    request.SetFailOnError(false);
  }

  request.SetRequestHeaders(request_headers.Get());
  request.SetVerifyPeer(false);

  try {
    request.StartIndirect();
    handler.Wait();
  } catch (const std::exception &exc) {
    success = false;
  }

  success &= fclose(file) == 0;

  if (!success) File::Delete(temp_path);

  file = NULL;

  if (success) {
    if (!File::Delete(final_path) && File::ExistsAny(final_path)) {
      File::Delete(temp_path);
      return false;
    }
    std::rename(temp_path.c_str(), args.path.c_str());
  }

  return success;
}

bool
SkysightRequest::RequestToBuffer(tstring &response)
{
  LogFormat("Connecting to %s with key %s", args.url.c_str(), key.c_str());

  bool success = true;

  char buffer[10240];
  BufferHandler handler(buffer, sizeof(buffer));
  CurlRequest request(*Net::curl, args.url.c_str(), handler);
  CurlSlist request_headers;
  char api_key_buffer[4096];
  snprintf(api_key_buffer, sizeof(api_key_buffer), "%s: %s", "X-API-Key", key.c_str());
  request_headers.Append(api_key_buffer);

  tstring pBody;
  if (username.length() && password.length()) {
    char content_type_buffer[4096];
    snprintf(content_type_buffer, sizeof(content_type_buffer), "%s: %s", "Content-Type", "application/json");
    request_headers.Append(content_type_buffer);
    NarrowString<1024> creds;
    creds.Format("{\"username\":\"%s\",\"password\":\"%s\"}",
		 username.c_str(), password.c_str());
    pBody = creds.c_str();
    request.SetRequestBody(pBody.c_str(), pBody.length());
    request.SetFailOnError(false);
  }

  request.SetRequestHeaders(request_headers.Get());
  request.SetVerifyPeer(false);

  try {
    request.StartIndirect();
    handler.Wait();
  } catch (const std::exception &exc) {
    success = false;
  }

  response = tstring(buffer,
		     buffer + handler.GetReceived() / sizeof(buffer[0]));
  return success;
}
