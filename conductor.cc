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
#include "examples/rtc_gw/conductor.h"

#include <memory>
#include <utility>
#include <vector>

#include "api/test/fakeconstraints.h"
#include "examples/rtc_gw/defaults.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/json.h"
#include "rtc_base/logging.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

#define DTLS_ON  true
#define DTLS_OFF false

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return
        new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() {
    RTC_LOG(INFO) << __FUNCTION__;
  }
  virtual void OnFailure(const std::string& error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << error;
  }

 protected:
  DummySetSessionDescriptionObserver() {}
  ~DummySetSessionDescriptionObserver() {}
};

Conductor::Conductor(PeerConnectionListener* client) : peer_id_(-1), client_(client) {
  client_->RegisterObserver(this);
}

Conductor::~Conductor() {
  RTC_DCHECK(peer_connection_.get() == NULL);
}

bool Conductor::connection_active() const {
  return peer_connection_.get() != NULL;
}

void Conductor::Close() {
  client_->SignOut();
  DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
  RTC_DCHECK(peer_connection_factory_.get() == NULL);
  RTC_DCHECK(peer_connection_.get() == NULL);
  // CustomAudioModule
  signaling_thread_ = new rtc::Thread();
  rtcgw::FileAudioDevice *audio_device_ = new rtcgw::FileAudioDevice("/audio/input_48K_16bits_pcm.raw", "/audio/recording.raw");
  signaling_thread_->Start();

  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
     signaling_thread_,
     rtc::Thread::Current(),
     rtc::Thread::Current(),
     audio_device_,
     webrtc::CreateBuiltinAudioEncoderFactory(),
     webrtc::CreateBuiltinAudioDecoderFactory(),
     nullptr,
     nullptr
  );

  if (!peer_connection_factory_.get()) {
    RTC_LOG(INFO) << "Error Failed to initialize PeerConnectionFactory";
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(DTLS_ON)) {
    RTC_LOG(INFO) << "Error: CreatePeerConnection failed";
    DeletePeerConnection();
  }
  AddStreams();
  return peer_connection_.get() != NULL;
}

bool Conductor::CreatePeerConnection(bool dtls) {
  RTC_DCHECK(peer_connection_factory_.get() != NULL);
  RTC_DCHECK(peer_connection_.get() == NULL);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.audio_jitter_buffer_max_packets = 100;
  config.audio_jitter_buffer_fast_accelerate = true;
  RTC_LOG(INFO) << "config.audio_jitter_buffer_fast_accelerate: " << config.audio_jitter_buffer_fast_accelerate;

  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = GetPeerConnectionString();
  config.servers.push_back(server);

  webrtc::FakeConstraints constraints;
  if (dtls) {
    constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                            "true");
    RTC_LOG(INFO) << __FUNCTION__ << " DTLS constraint: true ";
  } else {
    constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                            "false");
    RTC_LOG(INFO) << __FUNCTION__ << " DTLS constraint: false ";
  }
  constraints.AddOptional(webrtc::MediaConstraintsInterface::kOfferToReceiveVideo,
                            "false");
  RTC_LOG(INFO) << __FUNCTION__ << " offer receive video: false ";
  peer_connection_ = peer_connection_factory_->CreatePeerConnection(
      config, &constraints, NULL, NULL, this);
  return peer_connection_.get() != NULL;
}

void Conductor::DeletePeerConnection() {
  peer_connection_ = NULL;
  active_streams_.clear();
  peer_connection_factory_ = NULL;
  peer_id_ = -1;
}

void Conductor::EnsureStreamingUI() {
  RTC_DCHECK(peer_connection_.get() != NULL);
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Conductor::OnAddStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << stream->id();
    // QueueThreadCallback(NEW_STREAM_ADDED, stream.release());
}

void Conductor::OnRemoveStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << stream->id();
    // QueueThreadCallback(STREAM_REMOVED, stream.release());
}


void Conductor::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {

    RTC_LOG(INFO) << __FUNCTION__ << " ? " << webrtc::PeerConnectionInterface::kIceGatheringComplete << " == " << new_state;
};

void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    RTC_LOG(WARNING) << __FUNCTION__ << " " << candidate->sdp_mline_index();

//    std::vector<cricket::Candidate> candidates;
//    candidates.push_back(candidate->candidate());
//    peer_connection_->RemoveIceCandidates(candidates);

    std::string sdp;
    if (!candidate->ToString(&sdp)) {
        RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
        return;
    } else {
        RTC_LOG(WARNING) << "Ice Candidate:" << sdp;
    }

    std::size_t found = sdp.find(client_->listen_ip);
    if (found != std::string::npos) {
       RTC_LOG(WARNING) << "main "<< client_->listen_ip <<" Ice Candidate:" << sdp;
       std::string sdp;
       desc_->ToString(&sdp);
       QueueMessage(sdp);
       RTC_LOG(INFO) << __FUNCTION__ << "queuing:" << sdp;
    }
}

//
// PeerConnectionListenerObserver implementation.
//

void Conductor::OnSignedIn() {
  RTC_LOG(INFO) << __FUNCTION__;
}

void Conductor::OnDisconnected() {
  RTC_LOG(INFO) << __FUNCTION__;

  DeletePeerConnection();
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
  RTC_LOG(INFO) << __FUNCTION__;
}

void Conductor::OnPeerDisconnected(int id) {
  RTC_LOG(INFO) << __FUNCTION__;
  if (id == peer_id_) {
    RTC_LOG(INFO) << "Our peer disconnected";
      QueueMessage("peer disconnected");
      DeletePeerConnection();
      // QueueThreadCallback(PEER_CONNECTION_CLOSED, NULL);
  }
}

void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
  RTC_DCHECK(peer_id_ == peer_id || peer_id_ == -1);
  RTC_DCHECK(!message.empty());
  RTC_LOG(INFO) << __FUNCTION__ ;

  if (!peer_connection_.get()) {
    RTC_DCHECK(peer_id_ == -1);
    peer_id_ = peer_id;

    if (!InitializePeerConnection()) {
      RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    } else {
      RTC_LOG(LS_ERROR) << "Ok initialize our PeerConnection instance";
    }
  } else if (peer_id != peer_id_) {
    RTC_DCHECK(peer_id_ != -1);
    RTC_LOG(WARNING) << "Received a message from unknown peer while already in a "
                    "conversation with a different peer.";
    return;
  }

  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(message, jmessage)) {
    RTC_LOG(WARNING) << "Received unknown message. " << message;
    return;
  }
  std::string type;
  std::string json_object;

  rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
  if (!type.empty()) {
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                      &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    RTC_LOG(WARNING) <<"type["<<type<<"]sdp["<< sdp <<"]";
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription(type, sdp, &error));
    if (!session_description) {
      RTC_LOG(WARNING) << "Can't parse received session description message. "
          << "SdpParseError was: " << error.description;
      return;
    }
    RTC_LOG(INFO) << " Received session description :" << message;
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(), session_description);
    RTC_LOG(INFO) << " remote description set !";
    if (session_description->type() ==
        webrtc::SessionDescriptionInterface::kOffer) {
      peer_connection_->CreateAnswer(this, NULL);
      RTC_LOG(INFO) << " Answer created !";
    }
    return;
  } else {
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                      &sdp_mid) ||
        !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                   &sdp_mlineindex) ||
        !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
      RTC_LOG(WARNING) << "Can't parse received message.";
      return;
    }
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
      RTC_LOG(WARNING) << "Can't parse received candidate message. "
          << "SdpParseError was: " << error.description;
      return;
    }
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      RTC_LOG(WARNING) << "Failed to apply the received candidate";
      return;
    }
    RTC_LOG(INFO) << " Received candidate :" << message;
    return;
  }
}

void Conductor::OnMessageSent(int err) {
   // QueueThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
    RTC_LOG(INFO) << "Error Failed to connect to :" << server_;
}

void Conductor::StartListen(const std::string& ip, int port) {
  client_->listen_ip = ip;
  client_->Listen(ip, port);
}

void Conductor::DisconnectFromServer() {
  if (client_->is_connected())
    client_->SignOut();
}

void Conductor::ConnectToPeer(int peer_id) {
  RTC_DCHECK(peer_id_ == -1);
  RTC_DCHECK(peer_id != -1);

  if (peer_connection_.get()) {
    RTC_LOG(INFO) << "Error: We only support connecting to one peer at a time";
    return;
  }

  if (InitializePeerConnection()) {
    peer_id_ = peer_id;
    peer_connection_->CreateOffer(this, NULL);
  } else {
    RTC_LOG(INFO) << "Error Failed to initialize PeerConnection";
  }
}

void Conductor::AddStreams() {
  if (active_streams_.find("stream_id_todo_multi_stream") != active_streams_.end())
    return;  // Already added.

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel, peer_connection_factory_->CreateAudioSource(NULL)));

  rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
      peer_connection_factory_->CreateLocalMediaStream("stream_id_todo_multi_stream");

  stream->AddTrack(audio_track);
  if (!peer_connection_->AddStream(stream)) {
    RTC_LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
  }
  typedef std::pair<std::string,
                    rtc::scoped_refptr<webrtc::MediaStreamInterface> >
      MediaStreamPair;
  active_streams_.insert(MediaStreamPair(stream->id(), stream));
}

void Conductor::DisconnectFromCurrentPeer() {
  RTC_LOG(INFO) << __FUNCTION__;
  if (peer_connection_.get()) {
    client_->SendHangUp(peer_id_);
    DeletePeerConnection();
  }
}

// Not used since we got rid of GTK, finctionnality need to be moved one by one
void Conductor::ThreadCallback(int msg_id, void* data) {
  switch (msg_id) {
    case PEER_CONNECTION_CLOSED:
      RTC_LOG(INFO) << "PEER_CONNECTION_CLOSED";
      DeletePeerConnection();
      RTC_DCHECK(active_streams_.empty());
      break;
    case SEND_MESSAGE_TO_PEER: {
      break;
    }
    case NEW_STREAM_ADDED: {
      webrtc::MediaStreamInterface* stream =
          reinterpret_cast<webrtc::MediaStreamInterface*>(
          data);
      webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
      stream->Release();
      break;
    }
    case STREAM_REMOVED: {
      // Remote peer stopped sending a stream.
      webrtc::MediaStreamInterface* stream =
          reinterpret_cast<webrtc::MediaStreamInterface*>(
          data);
      stream->Release();
      break;
    }
    default:
      RTC_NOTREACHED();
      break;
  }
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);
  desc_ = desc;
  RTC_LOG(INFO) << __FUNCTION__ << " success SDP answer waiting for ICE candidate" ;
}

void Conductor::OnFailure(const std::string& error) {
   RTC_LOG(LERROR) << error;
}

void Conductor::QueueMessage(const std::string& json_object) {
   RTC_LOG(INFO) << __FUNCTION__ <<" peer:" << peer_id_;
   std::string* msg = new std::string(json_object);
   pending_messages_.push_back(msg);
}

void Conductor::SendMessage() {
   if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
      RTC_LOG(INFO) << __FUNCTION__ <<" peer:" << peer_id_;
      std::string *msg = pending_messages_.front();
      RTC_LOG(INFO) << __FUNCTION__ <<" msg:" << *msg;
      pending_messages_.pop_front();
      if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
         RTC_LOG(LS_ERROR) << "SendToPeer failed";
         DisconnectFromServer();
      }
      delete msg;
   }
}
