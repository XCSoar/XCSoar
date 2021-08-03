// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHTREQUEST_HPP
#define WEATHER_SKYSIGHTREQUEST_HPP

#include "APIGlue.hpp"
#include "thread/StandbyThread.hpp"
#include "util/tstring.hpp"

#include "system/Path.hpp"
#include "system/FileUtil.hpp"

// #include "Net/HTTP/Session.hpp"
#include "lib/curl/Request.hxx"
#include "lib/curl/Handler.hxx"

#include "io/FileLineReader.hpp"

#include "LogFile.hpp"

class SkysightRequestError {};

struct SkysightRequest {
public:
  enum class Status {Idle, Busy, Complete, Error};

  SkysightRequest(const SkysightRequestArgs &_args): args(_args) {};
  bool Process();
  bool ProcessToString(tstring &response);

  SkysightCallType GetType();
  void SetCredentials(const TCHAR *_key, const TCHAR *_username = nullptr,
                      const TCHAR *_password = nullptr);

  class FileHandler final: public CurlResponseHandler {
    FILE *file;
    size_t received = 0;
    Mutex mutex;
    Cond cond;

    std::exception_ptr error;

    bool done = false;

  public:
    FileHandler(FILE *_file): file(_file) {};
    // void DataReceived(const void *data, size_t length) override;
    // void ResponseReceived(int64_t content_length) override;
    void OnData(std::span<const std::byte> data) override;
    void OnHeaders(unsigned status, Curl::Headers &&headers) override;
    void OnEnd() override;
    void OnError(std::exception_ptr e) noexcept override;
    void Wait();
  };

  class BufferHandler final: public CurlResponseHandler {
    uint8_t *buffer;
    const size_t max_size;
    size_t received = 0;
    Mutex mutex;
    Cond cond;

    std::exception_ptr error;

    bool done = false;
    
  public:
    BufferHandler(void *_buffer, size_t _max_size):
      buffer((uint8_t *)_buffer), max_size(_max_size) {}
    size_t GetReceived() const;
    // void ResponseReceived(int64_t content_length) override;
    // void DataReceived(const void *data, size_t length) override;
    void OnData(std::span<const std::byte> data) override;
    void OnHeaders(unsigned status, Curl::Headers &&headers) override;
    void OnEnd() override;
    void OnError(std::exception_ptr e) noexcept override;
    void Wait();
  };

protected:
  const SkysightRequestArgs args;
  tstring key, username, password;
  bool RequestToFile();
  bool RequestToBuffer(tstring &response);
};

struct SkysightAsyncRequest final:
  public SkysightRequest, public StandbyThread {
public:
  SkysightAsyncRequest(const SkysightRequestArgs &_args):
    SkysightRequest(_args), StandbyThread("SkysightAPIRequest"),
    status(Status::Idle) {};

  Status GetStatus();

  void Process();
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
