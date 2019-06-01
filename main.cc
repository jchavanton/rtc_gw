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


#include "examples/rtc_gw/conductor.h"
#include "examples/rtc_gw/flagdefs.h"
#include "examples/rtc_gw/peer_connection_listener.h"

#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"

class CustomSocketServer : public rtc::PhysicalSocketServer {
 public:
  explicit CustomSocketServer() : conductor_(NULL), client_(NULL) {}
  virtual ~CustomSocketServer() {}

  void SetMessageQueue(rtc::MessageQueue* queue) override {
    message_queue_ = queue;
  }

  void set_client(PeerConnectionListener* client) { client_ = client; }
  void set_conductor(Conductor* conductor) { conductor_ = conductor; }

  virtual bool Wait(int cms, bool process_io) override {
    conductor_->SendMessage();
    return rtc::PhysicalSocketServer::Wait(10/*cms == -1 ? 1 : cms*/, process_io);
  }

 protected:
  rtc::MessageQueue* message_queue_;
  Conductor* conductor_;
  PeerConnectionListener* client_;
};

int main(int argc, char* argv[]) {
  printf("starting ...\n ");

  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
  rtc::FlagList::Print(NULL, false);
  if (FLAG_help) {
    rtc::FlagList::Print(NULL, false);
    return 0;
  }

  // Abort if the user specifies a port that is outside the allowed
  // range [1, 65535].
  if ((FLAG_port < 1) || (FLAG_port > 65535)) {
    printf("Error: %i is not a valid port.\n", FLAG_port);
    return -1;
  }

  printf("listening[%s]\n", FLAG_listen);
  CustomSocketServer socket_server;
  rtc::AutoSocketServerThread thread(&socket_server);

  rtc::InitializeSSL();
  // Must be constructed after we set the socketserver.
  PeerConnectionListener client;
  rtc::scoped_refptr<Conductor> conductor(new rtc::RefCountedObject<Conductor>(&client));
  socket_server.set_client(&client);
  socket_server.set_conductor(conductor);
  conductor->StartListen(FLAG_listen, FLAG_port);
  thread.Run();

  rtc::CleanupSSL();
  return 0;
}
