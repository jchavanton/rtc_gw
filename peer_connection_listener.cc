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

#include "examples/rtc_gw/peer_connection_listener.h"

#include "examples/rtc_gw/defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/net_helpers.h"
#include "rtc_base/string_utils.h"

#ifdef WIN32
#include "rtc_base/win32socketserver.h"
#endif

namespace {

// This is our magical hangup signal.
const char kByeMessage[] = "BYE";
// Delay between server connection retries, in milliseconds
const int kReconnectDelay = 2000;

rtc::AsyncSocket* CreateServerSocket(int family) {
  rtc::Thread* thread = rtc::Thread::Current();
  RTC_DCHECK(thread != NULL);
  return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
}

}  // namespace

PeerConnectionListener::PeerConnectionListener()
  : callback_(NULL),
    resolver_(NULL),
    state_(NOT_CONNECTED),
    my_id_(-1) {
}

PeerConnectionListener::~PeerConnectionListener() {
}

void PeerConnectionListener::InitSocketSignals() {
  RTC_DCHECK(control_socket_.get() != NULL);
  RTC_DCHECK(hanging_get_.get() != NULL);
  control_socket_->SignalCloseEvent.connect(this,
      &PeerConnectionListener::OnClose);
  hanging_get_->SignalCloseEvent.connect(this,
      &PeerConnectionListener::OnClose);
  control_socket_->SignalConnectEvent.connect(this,
      &PeerConnectionListener::OnConnect);
  hanging_get_->SignalConnectEvent.connect(this,
      &PeerConnectionListener::OnHangingGetConnect);
  control_socket_->SignalReadEvent.connect(this,
      &PeerConnectionListener::OnRead);
  hanging_get_->SignalReadEvent.connect(this,
      &PeerConnectionListener::OnHangingGetRead);
}

int PeerConnectionListener::id() const {
  return my_id_;
}

bool PeerConnectionListener::is_connected() const {
  return my_id_ != -1;
}

const Peers& PeerConnectionListener::peers() const {
  return peers_;
}

void PeerConnectionListener::RegisterObserver(
    PeerConnectionListenerObserver* callback) {
  RTC_DCHECK(!callback_);
  callback_ = callback;
}

bool PeerConnectionListener::SendToPeer(int peer_id, const std::string& message) {
  if (peer_id == 7) {
    char headers[1024];
    snprintf(headers, sizeof(headers),
    "HTTP/1.1 200 OK\r\n"
    "Server: RTC_GW/0.1\r\n"
    "Cache-Control: no-cache\r\n"
    "Content-Length: %i\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n",
      (int) message.length());
    std::string answer = headers;
    // answer += message.substr(14, message.length() - 40);
    answer += message; //.substr(14, message.length() - 40);
    size_t sent = server_new_socket_->Send(answer.data(), answer.length());
    RTC_LOG(INFO) << "sent:" << sent;
    server_new_socket_->Close();
  }
  return 1;
}

bool PeerConnectionListener::SendHangUp(int peer_id) {
  return SendToPeer(peer_id, kByeMessage);
}

bool PeerConnectionListener::IsSendingMessage() {
  return state_ == CONNECTED &&
         control_socket_->GetState() != rtc::Socket::CS_CLOSED;
}

bool PeerConnectionListener::SignOut() {
  if (state_ == NOT_CONNECTED || state_ == SIGNING_OUT)
    return true;

  if (hanging_get_->GetState() != rtc::Socket::CS_CLOSED)
    hanging_get_->Close();

  if (control_socket_->GetState() == rtc::Socket::CS_CLOSED) {
    state_ = SIGNING_OUT;

    if (my_id_ != -1) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
          "GET /sign_out?peer_id=%i HTTP/1.0\r\n\r\n", my_id_);
      onconnect_data_ = buffer;
      return ConnectControlSocket();
    } else {
      // Can occur if the app is closed before we finish connecting.
      return true;
    }
  } else {
    state_ = SIGNING_OUT_WAITING;
  }

  return true;
}

void PeerConnectionListener::Close() {
  control_socket_->Close();
  hanging_get_->Close();
  onconnect_data_.clear();
  peers_.clear();
  if (resolver_ != NULL) {
    resolver_->Destroy(false);
    resolver_ = NULL;
  }
  my_id_ = -1;
  state_ = NOT_CONNECTED;
}

bool PeerConnectionListener::ConnectControlSocket() {
  RTC_DCHECK(control_socket_->GetState() == rtc::Socket::CS_CLOSED);
  int err = control_socket_->Connect(server_address_);
  if (err == SOCKET_ERROR) {
    Close();
    return false;
  }
  return true;
}

// Server BEGIN
void PeerConnectionListener::InitServerSocketSignals() {
  RTC_DCHECK(server_socket_.get() != NULL);
  server_socket_->SignalConnectEvent.connect(this, &PeerConnectionListener::OnServerConnect);
  server_socket_->SignalReadEvent.connect(this, &PeerConnectionListener::OnServerRead);
  server_socket_->SignalCloseEvent.connect(this, &PeerConnectionListener::OnServerClose);
}

void PeerConnectionListener::InitServerNewSocketSignals() {
  //RTC_DCHECK(server_new_socket_->get() != NULL);
  server_new_socket_->SignalConnectEvent.connect(this, &PeerConnectionListener::OnServerConnect);
  server_new_socket_->SignalReadEvent.connect(this, &PeerConnectionListener::OnServerRead);
  server_new_socket_->SignalCloseEvent.connect(this, &PeerConnectionListener::OnServerClose);
}

void PeerConnectionListener::Listen(const std::string& server, int port) {
  RTC_DCHECK(!server.empty());
  listen_address_.SetIP(server);
  listen_address_.SetPort(port);
  if (port <= 0)
    port = kDefaultServerPort;
  server_socket_.reset(CreateServerSocket(listen_address_.ipaddr().family()));
  InitServerSocketSignals();
  int err = server_socket_->Bind(listen_address_);
  if (err == SOCKET_ERROR) {
     RTC_LOG(LS_ERROR) << __FUNCTION__ << ": error socket binding";
  }
  err = server_socket_->Listen(10);
  if (err == SOCKET_ERROR) {
     RTC_LOG(LS_ERROR) << __FUNCTION__ << ": error socket listen";
  }
  return;
}

void PeerConnectionListener::OnServerClose(rtc::AsyncSocket* socket, int err) {
   RTC_LOG(LS_INFO) << __FUNCTION__ << " error:"<< err;
}
void PeerConnectionListener::OnServerConnect(rtc::AsyncSocket* socket) {
   RTC_LOG(LS_INFO) << __FUNCTION__;
}

// (0)CLOSED (1)CONNECTING (2)CONNECTED
void PeerConnectionListener::OnServerRead(rtc::AsyncSocket* socket) {
   RTC_LOG(LS_INFO) << __FUNCTION__ <<": socket["<<socket<<"] state:"<< socket->GetState();
   if (socket->GetState() == 1) {
      rtc::SocketAddress address;
      server_new_socket_ = socket->Accept(&address);
      InitServerNewSocketSignals();
      if (!server_new_socket_) {
         RTC_LOG(LS_ERROR) << "TCP accept failed with error "
                       << server_new_socket_->GetError();
         return;
      }
   }

   size_t content_length = 0;
   if (ReadIntoBuffer(server_new_socket_, &request_data_, &content_length)) {
     RTC_LOG(LS_INFO) << __FUNCTION__ <<" received:"<< content_length <<" GetRequest...";
     GetRequest(request_data_);
     request_data_ = "";
     RTC_LOG(LS_INFO) << __FUNCTION__ <<" received:"<< content_length;
   } else {
       size_t found = request_data_.find("/BYE");
       if (found != std::string::npos) {
          RTC_LOG(LS_INFO) <<__FUNCTION__ <<" do[BYE]";
          callback_->OnPeerDisconnected(7);
          request_data_ = "";
          return;
       }
   }

   RTC_LOG(LS_INFO) << __FUNCTION__ <<"reading done, received:"<< content_length;
}

void PeerConnectionListener::OnConnect(rtc::AsyncSocket* socket) {
  RTC_DCHECK(!onconnect_data_.empty());
  size_t sent = socket->Send(onconnect_data_.c_str(), onconnect_data_.length());
  RTC_DCHECK(sent == onconnect_data_.length());
  onconnect_data_.clear();
}

void PeerConnectionListener::OnHangingGetConnect(rtc::AsyncSocket* socket) {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer),
           "GET /wait?peer_id=%i HTTP/1.0\r\n\r\n", my_id_);
  int len = static_cast<int>(strlen(buffer));
  int sent = socket->Send(buffer, len);
  RTC_DCHECK(sent == len);
}

void PeerConnectionListener::OnMessageFromPeer(int peer_id,
                                             const std::string& message) {
  if (message.length() == (sizeof(kByeMessage) - 1) &&
      message.compare(kByeMessage) == 0) {
    callback_->OnPeerDisconnected(peer_id);
  } else {
    callback_->OnMessageFromPeer(peer_id, message);
  }
}

bool PeerConnectionListener::GetHeaderValue(const std::string& data,
                                          size_t eoh,
                                          const char* header_pattern,
                                          size_t* value) {
  RTC_DCHECK(value != NULL);
  size_t found = data.find(header_pattern);
  if (found != std::string::npos && found < eoh) {
    *value = atoi(&data[found + strlen(header_pattern)]);
    return true;
  }
  return false;
}

bool PeerConnectionListener::GetHeaderValue(const std::string& data, size_t eoh,
                                          const char* header_pattern,
                                          std::string* value) {
  RTC_DCHECK(value != NULL);
  size_t found = data.find(header_pattern);
  if (found != std::string::npos && found < eoh) {
    size_t begin = found + strlen(header_pattern);
    size_t end = data.find("\r\n", begin);
    if (end == std::string::npos)
      end = eoh;
    value->assign(data.substr(begin, end - begin));
    return true;
  }
  return false;
}

bool PeerConnectionListener::ReadIntoBuffer(rtc::AsyncSocket* socket,
                                          std::string* data,
                                          size_t* content_length) {
  char buffer[0xffff];
  do {
    int bytes = socket->Recv(buffer, sizeof(buffer), nullptr);
    if (bytes <= 0)
      break;
    data->append(buffer, bytes);
  } while (true);

  bool ret = false;
  size_t i = data->find("\r\n\r\n");
  if (i != std::string::npos) {
    RTC_LOG(INFO) << "Headers received, Content-Length ?";
    if (GetHeaderValue(*data, i, "\r\nContent-Length: ", content_length)) {
      size_t total_response_size = (i + 4) + *content_length;
      if (data->length() >= total_response_size) {
        ret = true;
        std::string should_close;
        const char kConnection[] = "\r\nConnection: ";
        if (GetHeaderValue(*data, i, kConnection, &should_close) &&
            should_close.compare("close") == 0) {
          socket->Close();
          // Since we closed the socket, there was no notification delivered
          // to us.  Compensate by letting ourselves know.
          OnClose(socket, 0);
        }
      } else {
        RTC_LOG(INFO) << "More data ["<<data->length()<<"/"<<total_response_size<<"]";
        // We haven't received everything.  Just continue to accept data.
      }
    } else {
      RTC_LOG(LS_ERROR) << "No content length field specified by the server.";
    }
  }
  return ret;
}

void PeerConnectionListener::OnRead(rtc::AsyncSocket* socket) {
  size_t content_length = 0;
  if (ReadIntoBuffer(socket, &control_data_, &content_length)) {
    size_t peer_id = 0, eoh = 0;
    bool ok = ParseServerResponse(control_data_, content_length, &peer_id,
                                  &eoh);
    if (ok) {
      if (my_id_ == -1) {
        // First response.  Let's store our server assigned ID.
        RTC_DCHECK(state_ == SIGNING_IN);
        my_id_ = static_cast<int>(peer_id);
        RTC_DCHECK(my_id_ != -1);

        // The body of the response will be a list of already connected peers.
        if (content_length) {
          size_t pos = eoh + 4;
          while (pos < control_data_.size()) {
            size_t eol = control_data_.find('\n', pos);
            if (eol == std::string::npos)
              break;
            int id = 0;
            std::string name;
            bool connected;
            if (ParseEntry(control_data_.substr(pos, eol - pos), &name, &id,
                           &connected) && id != my_id_) {
              peers_[id] = name;
              callback_->OnPeerConnected(id, name);
            }
            pos = eol + 1;
          }
        }
        RTC_DCHECK(is_connected());
        callback_->OnSignedIn();
      } else if (state_ == SIGNING_OUT) {
        Close();
        callback_->OnDisconnected();
      } else if (state_ == SIGNING_OUT_WAITING) {
        SignOut();
      }
    }

    control_data_.clear();

    if (state_ == SIGNING_IN) {
      RTC_DCHECK(hanging_get_->GetState() == rtc::Socket::CS_CLOSED);
      state_ = CONNECTED;
      hanging_get_->Connect(server_address_);
    }
  }
}

void PeerConnectionListener::OnHangingGetRead(rtc::AsyncSocket* socket) {
  RTC_LOG(INFO) << __FUNCTION__;
  size_t content_length = 0;
  if (ReadIntoBuffer(socket, &notification_data_, &content_length)) {
    size_t peer_id = 0, eoh = 0;
    bool ok = ParseServerResponse(notification_data_, content_length,
                                  &peer_id, &eoh);

    if (ok) {
      // Store the position where the body begins.
      size_t pos = eoh + 4;

      if (my_id_ == static_cast<int>(peer_id)) {
        // A notification about a new member or a member that just
        // disconnected.
        int id = 0;
        std::string name;
        bool connected = false;
        if (ParseEntry(notification_data_.substr(pos), &name, &id,
                       &connected)) {
          if (connected) {
            peers_[id] = name;
            callback_->OnPeerConnected(id, name);
          } else {
            peers_.erase(id);
            callback_->OnPeerDisconnected(id);
          }
        }
      } else {
        OnMessageFromPeer(static_cast<int>(peer_id),
                          notification_data_.substr(pos));
      }
    }

    notification_data_.clear();
  }

  if (hanging_get_->GetState() == rtc::Socket::CS_CLOSED &&
      state_ == CONNECTED) {
    hanging_get_->Connect(server_address_);
  }
}

bool PeerConnectionListener::ParseEntry(const std::string& entry,
                                      std::string* name,
                                      int* id,
                                      bool* connected) {
  RTC_DCHECK(name != NULL);
  RTC_DCHECK(id != NULL);
  RTC_DCHECK(connected != NULL);
  RTC_DCHECK(!entry.empty());

  *connected = false;
  size_t separator = entry.find(',');
  if (separator != std::string::npos) {
    *id = atoi(&entry[separator + 1]);
    name->assign(entry.substr(0, separator));
    separator = entry.find(',', separator + 1);
    if (separator != std::string::npos) {
      *connected = atoi(&entry[separator + 1]) ? true : false;
    }
  }
  return !name->empty();
}

int PeerConnectionListener::GetResponseStatus(const std::string& response) {
  int status = -1;
  size_t pos = response.find(' ');
  if (pos != std::string::npos)
    status = atoi(&response[pos + 1]);
  return status;
}

bool PeerConnectionListener::GetRequest(const std::string& request) {
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" >> "<< request;
  size_t pos = request.find('/');
  if (pos != std::string::npos) {
    if (request.substr(pos+1, 5).compare("OFFER") == 0) {
       RTC_LOG(LS_INFO) <<__FUNCTION__ <<" do["<<request.substr(pos+1, 5) <<"]";

    } else {
       return false;
    }
  } else {
    return false;
  }
  size_t eoh = request.find("\r\n\r\n");
  RTC_DCHECK(eoh != std::string::npos);
  if (eoh == std::string::npos)
    return false;

  pos = eoh + 4;
  OnMessageFromPeer(static_cast<int>(7), request.substr(pos));
  return true;
}

bool PeerConnectionListener::ParseServerResponse(const std::string& response,
                                               size_t content_length,
                                               size_t* peer_id,
                                               size_t* eoh) {
  int status = GetResponseStatus(response.c_str());
  if (status != 200) {
    RTC_LOG(LS_ERROR) << "Received error from server wrong status:" << status;
    Close();
    callback_->OnDisconnected();
    return false;
  }

  *eoh = response.find("\r\n\r\n");
  RTC_DCHECK(*eoh != std::string::npos);
  if (*eoh == std::string::npos)
    return false;

  *peer_id = -1;

  // See comment in peer_channel.cc for why we use the Pragma header and
  // not e.g. "X-Peer-Id".
  GetHeaderValue(response, *eoh, "\r\nPragma: ", peer_id);

  return true;
}

void PeerConnectionListener::OnClose(rtc::AsyncSocket* socket, int err) {
  RTC_LOG(INFO) << __FUNCTION__;

  socket->Close();

#ifdef WIN32
  if (err != WSAECONNREFUSED) {
#else
  if (err != ECONNREFUSED) {
#endif
    if (socket == hanging_get_.get()) {
      if (state_ == CONNECTED) {
        hanging_get_->Close();
        hanging_get_->Connect(server_address_);
      }
    } else {
      callback_->OnMessageSent(err);
    }
  } else {
    if (socket == control_socket_.get()) {
      RTC_LOG(WARNING) << "Connection refused; retrying in 2 seconds";
      rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, kReconnectDelay, this,
                                          0);
    } else {
      Close();
      callback_->OnDisconnected();
    }
  }
}

void PeerConnectionListener::OnMessage(rtc::Message* msg) {
  // ignore msg; there is currently only one supported message ("retry")
  // // // // // // // // // DoConnect();
}
