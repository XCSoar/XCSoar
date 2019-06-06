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
#include "Metrics.hpp"
#include "Event/Timer.hpp"
#include "Time/BrokenDateTime.hpp"
#include <vector>



SkysightAPIQueue::~SkysightAPIQueue() { 
  Timer::Cancel();
}

void SkysightAPIQueue::AddRequest(std::unique_ptr<SkysightAsyncRequest> &&request, bool append_end) {

  if(!append_end) {
    //Login requests jump to the front of the queue
    request_queue.insert(request_queue.begin(), std::move(request));
  } else {
    request_queue.emplace_back(std::move(request));
  }
  if(!is_busy)
    Process();
}

void SkysightAPIQueue::AddDecodeJob(std::unique_ptr<CDFDecoder> &&job) {
  decode_queue.emplace_back(std::move(job));
  if(!is_busy)
    Process();
}
  

void SkysightAPIQueue::Process() {
  is_busy = true;
  
  if(!empty(request_queue)) {
    auto job = request_queue.begin();
    switch((*job)->GetStatus()) {
      case SkysightRequest::Status::Idle:
        
        //Provide the job with the very latest API key just prior to execution
        if((*job)->GetType() == SkysightCallType::Login) {
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

        if(!Timer::IsActive())
          Timer::Schedule(std::chrono::milliseconds(300));
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
  
  if(!empty(decode_queue)) {
    auto &&decode_job = decode_queue.begin();
     switch((*decode_job)->GetStatus()) {
      case CDFDecoder::Status::Idle:
        (*decode_job)->DecodeAsync();
        if(!Timer::IsActive())
          Timer::Schedule(std::chrono::milliseconds(300));
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


  if(empty(request_queue) && empty(decode_queue)) {
    Timer::Cancel();
  }
  is_busy = false;
 
}

void SkysightAPIQueue::Clear(const tstring &&msg) {
  Timer::Cancel();
  for(auto &&i = request_queue.begin(); i<request_queue.end();++i) {
    if((*i)->GetStatus() != SkysightRequest::Status::Busy) {
      (*i)->Done();
      (*i)->TriggerNullCallback(msg.c_str());
      request_queue.erase(i);
    }
  }
}

void SkysightAPIQueue::SetCredentials(const tstring &&_email, const tstring &&_pass) {
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



void SkysightAPIQueue::OnTimer() {
  Process();
}
