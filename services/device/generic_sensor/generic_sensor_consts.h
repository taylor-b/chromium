// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_GENERIC_SENSOR_CONSTS_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_GENERIC_SENSOR_CONSTS_H_

#define _USE_MATH_DEFINES
#include <math.h>

namespace device {

// Required for conversion from G/s^2 to m/s^2
constexpr double kMeanGravity = 9.80665;

// Required for conversion from deg to rad
constexpr double kRadiansInDegrees = M_PI / 180.0;

// Required for conversion from Gauss to uT.
constexpr double kMicroteslaInGauss = 100.0;

// Required for conversion from Milligauss to Microtesla.
constexpr double kMicroteslaInMilligauss = 0.1;

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_GENERIC_SENSOR_CONSTS_H_
