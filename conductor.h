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

#ifndef PEERCONNECTION_CONDUCTOR_H_
#define PEERCONNECTION_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "examples/rtc_gw/audio_device_module.h"
#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"
#include "examples/rtc_gw/peer_connection_listener.h"

class Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public PeerConnectionListenerObserver {

  public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_STREAM_ADDED,
    STREAM_REMOVED,
  };

  Conductor(PeerConnectionListener* client);

  bool connection_active() const;

  virtual void Close();

  void StartListen(const std::string& server, int port);
  void DisconnectFromServer();
  void ConnectToPeer(int peer_id);
  void DisconnectFromCurrentPeer();
  void ThreadCallback(int msg_id, void* data);

  // Send a message from the message queue if there is any
  void SendMessage();

 protected:
  rtc::Thread *worker_and_network_thread_;
  rtc::Thread *signaling_thread_;
  webrtc::AudioDeviceModule *audio_device_;
  int OnIceCandidateCount_;
  webrtc::SessionDescriptionInterface* desc_;
  ~Conductor();
  bool InitializePeerConnection();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddStreams();

  // PeerConnectionObserver implementation.
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override{};
  void OnAddStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnRemoveStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override{};
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  // PeerConnectionListenerObserver implementation.
  void OnSignedIn() override;
  void OnDisconnected() override;
  void OnPeerConnected(int id, const std::string& name) override;
  void OnPeerDisconnected(int id) override;
  void OnMessageFromPeer(int peer_id, const std::string& message) override;
  void OnMessageSent(int err) override;
  void OnServerConnectionFailure() override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(const std::string& error) override;

 protected:
  int peer_id_;
  // Queue a message to the remote peer.
  void QueueMessage(const std::string& json_object);

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionListener* client_;
  std::deque<std::string*> pending_messages_;
  std::map<std::string, rtc::scoped_refptr<webrtc::MediaStreamInterface> >
      active_streams_;
  std::string server_;
};

#endif  // PEERCONNECTION_CONDUCTOR_H_
