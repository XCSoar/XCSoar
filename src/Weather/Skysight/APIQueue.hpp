// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHTAPI_QUEUE_HPP
#define WEATHER_SKYSIGHTAPI_QUEUE_HPP

#include "Request.hpp"
#include "CDFDecoder.hpp"
#include "Metrics.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include <vector>

class SkysightAPIQueue final {
  std::vector<std::unique_ptr<SkysightAsyncRequest>> request_queue;
  std::vector<std::unique_ptr<CDFDecoder>> decode_queue;
  bool is_busy = false;
  bool is_clearing = false;
  tstring key;
  uint64_t key_expiry_time = 0;
  tstring email;
  tstring password;

  void Process();
  UI::PeriodicTimer timer{[this]{ Process(); }};

public:
  SkysightAPIQueue() {};
  ~SkysightAPIQueue();

  void SetCredentials(const tstring _email, const tstring _pass);
  void SetKey(const tstring _key, const uint64_t _key_expiry_time);
  bool IsLoggedIn();
  void AddRequest(std::unique_ptr<SkysightAsyncRequest> request,
      bool append_end = true);
  void AddDecodeJob(std::unique_ptr<CDFDecoder> &&job);
  void Clear(const tstring msg);
  void DoClearingQueue();
};

#endif
