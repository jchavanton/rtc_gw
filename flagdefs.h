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

#ifndef RTC_GW_FLAGDEFS_H_
#define RTC_GW_FLAGDEFS_H_

#include "rtc_base/flags.h"

extern const uint16_t kDefaultServerPort;  // From defaults.[h|cc]

// Define flags for the peerconnect_client testing tool, in a separate
// header file so that they can be shared across the different main.cc's
// for each platform.

DEFINE_bool(help, false, "Prints this message");
DEFINE_int(port, kDefaultServerPort, "The port on which the server is listening.");
DEFINE_string(listen, "localhost", "The IP to listen on.");

#endif  // RTC_GW_FLAGDEFS_H_
