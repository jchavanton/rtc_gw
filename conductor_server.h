/*
 *  Copyright 2017-2018 Julien Chavanton
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_GW_CONDUCTOR_SERVER_H_
#define RTC_GW_CONDUCTOR_SERVER_H_
#include "examples/rtc_gw/conductor.h"
#include <iostream>


class ConductorServer {
  public:
    ConductorServer(PeerConnectionListener* client);
    ~ConductorServer();
    Conductor* get_conductor() { return conductor_; };
  friend std::ostream& operator<<(std::ostream& os, const ConductorServer& cs) {
    os << cs.client_ << std::endl;
    return os; 
  };    
  protected:
    int peer_id_;
  private:
    PeerConnectionListener* client_;
    Conductor* conductor_;
    // std::vector<Conductor> Conductors;
};

#endif // RTC_GW_CONDUCTOR_SERVER_H_
