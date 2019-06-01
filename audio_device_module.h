/*
 *  Copyright (c) 2017-2018 Julien Chavanton
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef AUDIO_DEVICE_MODULE_H_
#define AUDIO_DEVICE_MODULE_H_

#include <stdio.h>

#include <memory>
#include <string>

#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/system/file_wrapper.h"

namespace rtc {
class PlatformThread;
}  // namespace rtc

namespace rtcgw {
class EventWrapper;

// This is a fake audio device which plays audio from a file as its microphone
// and plays out into a file.
//class FileAudioDevice : public webrtc::AudioDeviceGeneric {
class FileAudioDevice : public webrtc::AudioDeviceModule {
 public:
  // Constructs a file audio device with |id|. It will read audio from
  // |inputFilename| and record output audio to |outputFilename|.
  //
  // The input file should be a readable 48k stereo raw file, and the output
  // file should point to a writable location. The output format will also be
  // 48k stereo raw audio.
  FileAudioDevice(const char* inputFilename,
                  const char* outputFilename);
  virtual ~FileAudioDevice();

  webrtc::AudioDeviceBuffer *Audio_device_buffer_;

  // Retrieve the currently utilized audio layer
  int32_t ActiveAudioLayer(
      webrtc::AudioDeviceModule::AudioLayer* audioLayer) const ;

  // Main initializaton and termination
  int32_t Init() ;
  int32_t Terminate() ;
  bool Initialized() const ;

  // Device enumeration
  int16_t PlayoutDevices() ;
  int16_t RecordingDevices() ;
  int32_t PlayoutDeviceName(uint16_t index,
                            char name[webrtc::kAdmMaxDeviceNameSize],
                            char guid[webrtc::kAdmMaxGuidSize]) ;
  int32_t RecordingDeviceName(uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]) ;

  // Device selection
  int32_t SetPlayoutDevice(uint16_t index) ;
  int32_t SetPlayoutDevice(
      webrtc::AudioDeviceModule::WindowsDeviceType device) ;
  int32_t SetRecordingDevice(uint16_t index) ;
  int32_t SetRecordingDevice(
      webrtc::AudioDeviceModule::WindowsDeviceType device) ;

  // Audio transport initialization
  int32_t PlayoutIsAvailable(bool* available) ;
  int32_t InitPlayout() ;
  bool PlayoutIsInitialized() const ;
  int32_t RecordingIsAvailable(bool* available) ;
  int32_t InitRecording() ;
  bool RecordingIsInitialized() const ;

  // Audio transport control
  int32_t StartPlayout() ;
  int32_t StopPlayout() ;
  bool Playing() const ;
  int32_t StartRecording() ;
  int32_t StopRecording() ;
  bool Recording() const ;

  // Audio mixer initialization
  int32_t InitSpeaker() ;
  bool SpeakerIsInitialized() const ;
  int32_t InitMicrophone() ;
  bool MicrophoneIsInitialized() const ;

  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable(bool* available) ;
  virtual int32_t SetSpeakerVolume(uint32_t volume) ;
  virtual int32_t SpeakerVolume(uint32_t* volume) const ;
  virtual int32_t MaxSpeakerVolume(uint32_t* maxVolume) const ;
  virtual int32_t MinSpeakerVolume(uint32_t* minVolume) const ;

  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable(bool* available) ;
  virtual int32_t SetMicrophoneVolume(uint32_t volume) ;
  virtual int32_t MicrophoneVolume(uint32_t* volume) const ;
  virtual int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const ;
  virtual int32_t MinMicrophoneVolume(uint32_t* minVolume) const ;

  // Speaker mute control
  virtual int32_t SpeakerMuteIsAvailable(bool* available) ;
  virtual int32_t SetSpeakerMute(bool enable) ;
  virtual int32_t SpeakerMute(bool* enabled) const ;

  // Microphone mute control
  virtual int32_t MicrophoneMuteIsAvailable(bool* available) ;
  virtual int32_t SetMicrophoneMute(bool enable) ;
  virtual int32_t MicrophoneMute(bool* enabled) const ;


//../../modules/audio_device/include/audio_device.h:138:19: i
// note: unimplemented pure virtual method 'StereoRecordingIsAvailable'
//  in 'FileAudioDevice'
//  virtual int32_t StereoRecordingIsAvailable(bool* available) const = 0;

  // Stereo support
  virtual int32_t StereoPlayoutIsAvailable(bool* available) const;
  virtual int32_t SetStereoPlayout(bool enable);
  virtual int32_t StereoPlayout(bool* enabled) const;
  virtual int32_t StereoRecordingIsAvailable(bool* available) const;
  virtual int32_t SetStereoRecording(bool enable);
  virtual int32_t StereoRecording(bool* enabled) const;

  // Delay information and control
  virtual int32_t PlayoutDelay(uint16_t* delayMS) const;

  virtual void AttachAudioBuffer(webrtc::AudioDeviceBuffer* audioBuffer) ;

  // Extra features
  virtual bool BuiltInAECIsAvailable() const { return false; }
  virtual int32_t EnableBuiltInAEC(bool enable) { return -1; }
  virtual bool BuiltInAGCIsAvailable() const { return false; }
  virtual int32_t EnableBuiltInAGC(bool enable) { return -1; }
  virtual bool BuiltInNSIsAvailable() const { return false; }
  virtual int32_t EnableBuiltInNS(bool enable) { return -1; }

  //
  virtual void AddRef() const { return; }
  virtual int32_t RegisterAudioCallback(webrtc::AudioTransport* audio_callback);
  virtual rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

 private:
  static bool RecThreadFunc(void*);
  static bool PlayThreadFunc(void*);
  bool RecThreadProcess();
  bool PlayThreadProcess();

  int32_t _playout_index;
  int32_t _record_index;
  webrtc::AudioDeviceBuffer* _ptrAudioBuffer;
  int8_t* _recordingBuffer;  // In bytes.
  int8_t* _playoutBuffer;  // In bytes.
  uint32_t _recordingFramesLeft;
  uint32_t _playoutFramesLeft;
  rtc::CriticalSection _critSect;

  size_t _recordingBufferSizeIn10MS;
  size_t _recordingFramesIn10MS;
  size_t _playoutFramesIn10MS;

  // TODO(pbos): Make plain members instead of pointers and stop resetting them.
  std::unique_ptr<rtc::PlatformThread> _ptrThreadRec;
  std::unique_ptr<rtc::PlatformThread> _ptrThreadPlay;

  bool _playing;
  bool _recording;
  int64_t _lastCallPlayoutMillis;
  int64_t _lastCallRecordMillis;

  webrtc::FileWrapper& _outputFile;
  webrtc::FileWrapper& _inputFile;
  std::string _outputFilename;
  std::string _inputFilename;
};

}  // namespace webrtc

#endif  // AUDIO_DEVICE_MODULE_H_
