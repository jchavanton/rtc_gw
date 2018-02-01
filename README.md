


# Installation :

## WebRTC
#### depot_tools
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:/path/to/depot_tools
```
#### WebRTC sources
```
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc
gclient sync
cd src
git checkout master
gn gen out/Default
```


## RTC Gateway

### 
```
cd webrtc-checkout/src/examples
git clone https://github.com/jchavanton/rtc_gw.git
```

### Edit
```
../webrtc-checkout/src/BUILD.gn
```

### Add the following to the examples section  
```
group("examples") {
...


  if (is_linux) {
    deps += [
      ":rtc_gw",
    ]
  }

...
rtc_executable("rtc_gw") {
    testonly = true
    sources = [
      "rtc_gw/conductor.cc",
      "rtc_gw/conductor.h",
      "rtc_gw/defaults.cc",
      "rtc_gw/defaults.h",
      "rtc_gw/audio_device_module.cc",
      "rtc_gw/audio_device_module.h",
      "rtc_gw/peer_connection_listener.cc",
      "rtc_gw/peer_connection_listener.h",
      "rtc_gw/main.cc",
    ]

    if (!build_with_chromium && is_clang) {
      # Suppress warnings from the Chromium Clang plugin (bugs.webrtc.org/163).
      suppressed_configs += [ "//build/config/clang:find_bad_constructs" ]
    }


    cflags += [ "-Wno-inconsistent-missing-override" ]
    cflags += [ "-Wno-deprecated-declarations" ]
    deps = []

    configs += [ ":peerconnection_client_warnings_config" ]

    deps += [
      "../api:libjingle_peerconnection_test_api",
      "../api:peerconnection_and_implicit_call_api",
      "../api:video_frame_api",
      "../api/audio_codecs:builtin_audio_decoder_factory",
      "../api/audio_codecs:builtin_audio_encoder_factory",
      "../media:rtc_audio_video",
      "../modules/video_capture:video_capture_module",
      "../pc:libjingle_peerconnection",
      "../rtc_base:rtc_base",
      "../rtc_base:rtc_base_approved",
      "../rtc_base:rtc_json",
      "../system_wrappers:field_trial_default",
      "../system_wrappers:metrics_default",
      "../system_wrappers:runtime_enabled_features_default",
      "//third_party/libyuv",
    ]
  }
}
```
