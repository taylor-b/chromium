// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_DOWNLOAD_SERVICE_IMPL_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_DOWNLOAD_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "components/download/internal/config.h"
#include "components/download/internal/service_config_impl.h"
#include "components/download/public/download_service.h"

namespace download {

class Controller;
struct DownloadParams;
struct SchedulingParams;

// The internal implementation of the DownloadService.
class DownloadServiceImpl : public DownloadService {
 public:
  DownloadServiceImpl(std::unique_ptr<Configuration> config,
                      std::unique_ptr<Controller> controller);
  ~DownloadServiceImpl() override;

  // DownloadService implementation.
  const ServiceConfig& GetConfig() override;
  void OnStartScheduledTask(DownloadTaskType task_type,
                            const TaskFinishedCallback& callback) override;
  bool OnStopScheduledTask(DownloadTaskType task_type) override;
  ServiceStatus GetStatus() override;
  void StartDownload(const DownloadParams& download_params) override;
  void PauseDownload(const std::string& guid) override;
  void ResumeDownload(const std::string& guid) override;
  void CancelDownload(const std::string& guid) override;
  void ChangeDownloadCriteria(const std::string& guid,
                              const SchedulingParams& params) override;

 private:
  void OnControllerInitialized();

  // config_ needs to be destructed after controller_ and service_config_ which
  // hold onto references to it.
  std::unique_ptr<Configuration> config_;

  std::unique_ptr<Controller> controller_;
  ServiceConfigImpl service_config_;

  base::circular_deque<base::Closure> pending_actions_;
  std::map<DownloadTaskType, base::Closure> pending_tasks_;
  bool startup_completed_;

  DISALLOW_COPY_AND_ASSIGN(DownloadServiceImpl);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_DOWNLOAD_SERVICE_IMPL_H_
