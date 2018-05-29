/*
 *  copyright 2017-2018 Julien Chavanton
 *  copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "examples/rtc_gw/conductor_server.h"

ConductorServer::ConductorServer(PeerConnectionListener* client) : peer_id_(-1), client_(client) {
  conductor_ = new Conductor(client_);
  // client_->RegisterObserver(this);
}

ConductorServer::~ConductorServer() {
  // RTC_DCHECK(peer_connection_.get() == NULL);
}

//ConductorServer::get_conductor() {
//  return conductor_;
//}
