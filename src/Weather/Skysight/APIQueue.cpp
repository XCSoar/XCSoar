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
#include "SkysightAPI.hpp"
#include "APIQueue.hpp"
#include "APIGlue.hpp"
#include "Request.hpp"
#include "CDFDecoder.hpp"
#include "Layers.hpp"
#include "Event/PeriodicTimer.hpp"
#include "Time/BrokenDateTime.hpp"

void SkysightAPIQueue::AddRequest(std::unique_ptr<SkysightAsyncRequest> request, bool append_end) {
  
  if(!append_end) {
    //Login requests jump to the front of the queue
    request_deque.push_front(std::move(request));
  } else {
    request_deque.push_back(std::move(request));
  }
  if(!is_busy)
    Process();
}

void SkysightAPIQueue::AddDecodeJob(std::unique_ptr<CDFDecoder> job) {
  decode_queue.push(std::move(job));
  if(!is_busy)
    Process();
}

void SkysightAPIQueue::Process() {
  is_busy = true;
  
  if(!request_deque.empty()) {
    auto request = request_deque.front().get();
    switch(request->GetStatus()) {
      case SkysightRequest::Status::Idle:
        
        //Provide the request with the very latest API key just prior to execution
        if(request->GetType() == SkysightCallType::Login) {
          request->SetCredentials("XCSoar", email.c_str(), password.c_str());
          request->Process();
        } else {
          if (!IsLoggedIn()) {
            // inject a login request at the front of the queue
            SkysightAPI::GenerateLoginRequest();
          } else {
            request->SetCredentials(key.c_str());
            request->Process();
          }
        }

        if(!timer.IsActive())
          timer.Schedule(std::chrono::milliseconds(300));
        break;
      case SkysightRequest::Status::Complete:
      case SkysightRequest::Status::Error:
        request->Done();
        request_deque.pop_front();
        break;
      case SkysightRequest::Status::Busy:
        break;
    }
  }
  
  if(!decode_queue.empty()) {
    auto decode_job = decode_queue.front().get();
    if(!decode_job)
      return;
     switch(decode_job->GetStatus()) {
      case CDFDecoder::Status::Idle:
        decode_job->DecodeAsync();
        if(!timer.IsActive())
          timer.Schedule(std::chrono::milliseconds(300));
        break;
      case CDFDecoder::Status::Complete:
      case CDFDecoder::Status::Error:
        decode_job->Done();
        decode_queue.pop();
        break;
      case CDFDecoder::Status::Busy:
        break;
    }
  }

  if(request_deque.empty() && decode_queue.empty()) {
    timer.Cancel();
  }
  is_busy = false;
 
}

void SkysightAPIQueue::Clear(const tstring &&msg) {
  timer.Cancel();
  for(u_int i = 0; i < request_deque.size(); i++){
    SkysightAsyncRequest *request = request_deque.front().get();
    if(request->GetStatus() != SkysightRequest::Status::Busy) {
      request->Done();
      request->TriggerNullCallback(msg.c_str());
      request_deque.pop_front();
    }
  }
}

void SkysightAPIQueue::SetCredentials(const tstring _email, const tstring _pass) {
  password = _pass;
  email = _email;
}

void SkysightAPIQueue::SetKey(const tstring &&_key, const uint64_t _key_expiry_time) {
  key = _key;
  key_expiry_time = _key_expiry_time;
}

bool SkysightAPIQueue::IsLoggedIn() {
  
  uint64_t now = (uint64_t)BrokenDateTime::NowUTC().ToUnixTimeUTC();

  //Add a 2-minute padding so that token doesn't expire mid-way thru a request
  return ( ((int64_t)(key_expiry_time - now)) > (60*2) );
  
}