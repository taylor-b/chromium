// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_RATE_COUNTER_H_
#define REMOTING_BASE_RATE_COUNTER_H_

#include <stdint.h>

#include <queue>
#include <utility>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"

namespace remoting {

// Measures average rate per second of a sequence of point rate samples
// over a specified time window. This can be used to measure bandwidth, frame
// rates, etc.
class RateCounter {
 public:
  // Constructs a rate counter over the specified |time_window|.
  explicit RateCounter(base::TimeDelta time_window);
  virtual ~RateCounter();

  // Records a point event count to include in the rate.
  void Record(int64_t value);

  // Returns the rate-per-second of values recorded over the time window.
  // Note that rates reported before |time_window| has elapsed are not accurate.
  double Rate();

  void set_tick_clock_for_tests(base::TickClock* tick_clock) {
    tick_clock_ = tick_clock;
  }

 private:
  // Type used to store data points with timestamps.
  typedef std::pair<base::TimeTicks, int64_t> DataPoint;

  // Removes data points more than |time_window| older than |current_time|.
  void EvictOldDataPoints(base::TimeTicks current_time);

  // Time window over which to calculate the rate.
  const base::TimeDelta time_window_;

  // Queue containing data points in the order in which they were recorded.
  std::queue<DataPoint> data_points_;

  // Sum of values in |data_points_|.
  int64_t sum_;

  base::DefaultTickClock default_tick_clock_;
  base::TickClock* tick_clock_ = &default_tick_clock_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RateCounter);
};

}  // namespace remoting

#endif  // REMOTING_BASE_RATE_COUNTER_H_
