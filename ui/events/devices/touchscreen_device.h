// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_DEVICES_TOUCHSCREEN_DEVICE_H_
#define UI_EVENTS_DEVICES_TOUCHSCREEN_DEVICE_H_

#include <stdint.h>

#include <string>

#include "ui/display/types/display_constants.h"
#include "ui/events/devices/events_devices_export.h"
#include "ui/events/devices/input_device.h"
#include "ui/gfx/geometry/size.h"

namespace ui {

// Represents a Touchscreen device state.
struct EVENTS_DEVICES_EXPORT TouchscreenDevice : public InputDevice {
  // Creates an invalid touchscreen device.
  TouchscreenDevice();

  TouchscreenDevice(int id,
                    InputDeviceType type,
                    const std::string& name,
                    const gfx::Size& size,
                    int touch_points);

  TouchscreenDevice(const InputDevice& input_device,
                    const gfx::Size& size,
                    int touch_points);

  ~TouchscreenDevice() override;

  gfx::Size size;    // Size of the touch screen area.
  int touch_points;  // The number of touch points this device supports (0 if
                     // unknown).
  // True if the specified touchscreen device is stylus capable.
  bool is_stylus = false;
  // Id of the display the touch device targets.
  // NOTE: when obtaining TouchscreenDevice from InputDeviceManager this value
  // may not have been updated. See
  // InputDeviceManager::AreTouchscreenTargetDisplaysValid() for details.
  int64_t target_display_id = display::kInvalidDisplayId;
};

}  // namespace ui

#endif  // UI_EVENTS_DEVICES_TOUCHSCREEN_DEVICE_H_
