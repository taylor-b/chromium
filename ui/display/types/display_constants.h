// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_TYPES_DISPLAY_CONSTANTS_H_
#define UI_DISPLAY_TYPES_DISPLAY_CONSTANTS_H_

#include <stdint.h>

namespace display {

// Display ID that represents an invalid display. Often used as a default value
// before display IDs are known.
constexpr int64_t kInvalidDisplayId = -1;

// Used to describe the state of a multi-display configuration.
enum MultipleDisplayState {
  MULTIPLE_DISPLAY_STATE_INVALID,
  MULTIPLE_DISPLAY_STATE_HEADLESS,
  MULTIPLE_DISPLAY_STATE_SINGLE,
  MULTIPLE_DISPLAY_STATE_DUAL_MIRROR,
  MULTIPLE_DISPLAY_STATE_DUAL_EXTENDED,
  // TODO(oshima): consolidate DUAL_EXTENDED and MULTI_EXTENDED.
  MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
};

// Video output types.
enum DisplayConnectionType {
  DISPLAY_CONNECTION_TYPE_NONE = 0,
  DISPLAY_CONNECTION_TYPE_UNKNOWN = 1 << 0,
  DISPLAY_CONNECTION_TYPE_INTERNAL = 1 << 1,
  DISPLAY_CONNECTION_TYPE_VGA = 1 << 2,
  DISPLAY_CONNECTION_TYPE_HDMI = 1 << 3,
  DISPLAY_CONNECTION_TYPE_DVI = 1 << 4,
  DISPLAY_CONNECTION_TYPE_DISPLAYPORT = 1 << 5,
  DISPLAY_CONNECTION_TYPE_NETWORK = 1 << 6,

  // Update this when adding a new type.
  DISPLAY_CONNECTION_TYPE_LAST = DISPLAY_CONNECTION_TYPE_NETWORK
};

// Content protection methods applied on video output.
enum ContentProtectionMethod {
  CONTENT_PROTECTION_METHOD_NONE = 0,
  CONTENT_PROTECTION_METHOD_HDCP = 1 << 0,
};

// HDCP protection state.
enum HDCPState {
  HDCP_STATE_UNDESIRED,
  HDCP_STATE_DESIRED,
  HDCP_STATE_ENABLED,

  // Update this when adding a new type.
  HDCP_STATE_LAST = HDCP_STATE_ENABLED
};

}  // namespace display

#endif  // UI_DISPLAY_TYPES_DISPLAY_CONSTANTS_H_
