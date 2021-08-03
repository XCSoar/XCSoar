// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "APIQueue.hpp"
#include "APIGlue.hpp"
#include "Request.hpp"
#include "CDFDecoder.hpp"
#include "Metrics.hpp"
#include "ui/event/Timer.hpp"
#include "time/BrokenDateTime.hpp"
#include "LogFile.hpp"
#include <vector>

SkysightAPIQueue::~SkysightAPIQueue() {
	LogFormat("SkysightAPIQueue::~SkysightAPIQueue %d", timer.IsActive());
  timer.Cancel();
}

void
SkysightAPIQueue::AddRequest(std::unique_ptr<SkysightAsyncRequest> request,
			     bool append_end)
{
  if (!append_end) {
    //Login requests jump to the front of the queue
    request_queue.insert(request_queue.begin(), std::move(request));
  } else {
    request_queue.emplace_back(std::move(request));
  }
  if (!is_busy)
    Process();
}

void SkysightAPIQueue::AddDecodeJob(std::unique_ptr<CDFDecoder> &&job) {
  decode_queue.emplace_back(std::move(job));
  if(!is_busy)
    Process();
}

void SkysightAPIQueue::Process()
{
  is_busy = true;

  if(is_clearing) {
    DoClearingQueue();
  } else if (!request_queue.empty()) {
    auto job = request_queue.begin();
    switch ((*job)->GetStatus()) {
    case SkysightRequest::Status::Idle:
      //Provide the job with the very latest API key just prior to execution
      if ((*job)->GetType() == SkysightCallType::Login) {
        (*job)->SetCredentials("XCSoar", email.c_str(), password.c_str());
        (*job)->Process();
      } else {
        if (!IsLoggedIn()) {
          // inject a login request at the front of the queue
          SkysightAPI::GenerateLoginRequest();
        } else {
          (*job)->SetCredentials(key.c_str());
          (*job)->Process();
        }
      }

      if (!timer.IsActive())
	      timer.Schedule(std::chrono::milliseconds(300));
      break;
    case SkysightRequest::Status::Complete:
    case SkysightRequest::Status::Error:
      (*job)->Done();
      request_queue.erase(job);
      break;
    case SkysightRequest::Status::Busy:
      break;
    }
  }

  if (!empty(decode_queue)) {
    auto &&decode_job = decode_queue.begin();
    switch ((*decode_job)->GetStatus()) {
    case CDFDecoder::Status::Idle:
      (*decode_job)->DecodeAsync();
      if (!timer.IsActive())
	      timer.Schedule(std::chrono::milliseconds(300));
      break;
    case CDFDecoder::Status::Complete:
    case CDFDecoder::Status::Error:
      (*decode_job)->Done();
      decode_queue.erase(decode_job);
      break;
    case CDFDecoder::Status::Busy:
      break;
    }
  }

  if (empty(request_queue) && empty(decode_queue))
    timer.Cancel();

  is_busy = false;
}

void
SkysightAPIQueue::Clear(const tstring msg)
{
  LogFormat("SkysightAPIQueue::Clear %s", msg.c_str());
  is_clearing = true;
}

void
SkysightAPIQueue::SetCredentials(const tstring _email,
				 const tstring _pass)
{
  password = _pass;
  email = _email;
}

void
SkysightAPIQueue::SetKey(const tstring _key,
			 const uint64_t _key_expiry_time)
{
  key = _key;
  key_expiry_time = _key_expiry_time;
}

bool
SkysightAPIQueue::IsLoggedIn()
{
  uint64_t now = (uint64_t) std::chrono::system_clock::to_time_t(
    BrokenDateTime::NowUTC().ToTimePoint());

  //Add a 2-minute padding so that token doesn't expire mid-way thru a request
  return (((int64_t)(key_expiry_time - now)) > (60*2));
}

void
SkysightAPIQueue::DoClearingQueue()
{
  for (auto &&i = request_queue.begin(); i<request_queue.end(); ++i) {
    if ((*i)->GetStatus() != SkysightRequest::Status::Busy) {
      (*i)->Done();
      request_queue.erase(i);
    }
  }
  timer.Cancel();
  is_clearing = false;
}
