// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_H_

namespace offline_pages {

class PrefetchBackgroundTask {
 public:
  // API for interacting with BackgroundTaskScheduler from native.
  // Schedules the default 'NWake' prefetching task.
  // |additional_delay_seconds| is relative to the default 15 minute delay.
  // Implemented in platform-specific object files.
  static void Schedule(int additional_delay_seconds, bool update_current);

  // Cancels the default 'NWake' prefetching task.
  // Implemented in platform-specific object files.
  static void Cancel();
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_H_
