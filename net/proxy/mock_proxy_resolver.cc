// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/mock_proxy_resolver.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace net {

MockAsyncProxyResolver::RequestImpl::RequestImpl(std::unique_ptr<Job> job)
    : job_(std::move(job)) {
  DCHECK(job_);
}

MockAsyncProxyResolver::RequestImpl::~RequestImpl() {
  MockAsyncProxyResolver* resolver = job_->Resolver();
  // AddCancelledJob will check if request is already cancelled
  resolver->AddCancelledJob(std::move(job_));
}

LoadState MockAsyncProxyResolver::RequestImpl::GetLoadState() {
  return LOAD_STATE_RESOLVING_PROXY_FOR_URL;
}

MockAsyncProxyResolver::Job::Job(MockAsyncProxyResolver* resolver,
                                 const GURL& url,
                                 ProxyInfo* results,
                                 const CompletionCallback& callback)
    : resolver_(resolver), url_(url), results_(results), callback_(callback) {}

MockAsyncProxyResolver::Job::~Job() {}

void MockAsyncProxyResolver::Job::CompleteNow(int rv) {
  CompletionCallback callback = callback_;

  resolver_->RemovePendingJob(this);

  callback.Run(rv);
}

MockAsyncProxyResolver::~MockAsyncProxyResolver() {}

int MockAsyncProxyResolver::GetProxyForURL(
    const GURL& url,
    ProxyInfo* results,
    const CompletionCallback& callback,
    std::unique_ptr<Request>* request,
    const NetLogWithSource& /*net_log*/) {
  std::unique_ptr<Job> job(new Job(this, url, results, callback));

  pending_jobs_.push_back(job.get());
  request->reset(new RequestImpl(std::move(job)));

  // Test code completes the request by calling job->CompleteNow().
  return ERR_IO_PENDING;
}

void MockAsyncProxyResolver::AddCancelledJob(std::unique_ptr<Job> job) {
  std::vector<Job*>::iterator it =
      std::find(pending_jobs_.begin(), pending_jobs_.end(), job.get());
  // Because this is called always when RequestImpl is destructed,
  // we need to check if it is still in pending jobs.
  if (it != pending_jobs_.end()) {
    cancelled_jobs_.push_back(std::move(job));
    pending_jobs_.erase(it);
  }
}

void MockAsyncProxyResolver::RemovePendingJob(Job* job) {
  DCHECK(job);
  std::vector<Job*>::iterator it =
      std::find(pending_jobs_.begin(), pending_jobs_.end(), job);
  DCHECK(it != pending_jobs_.end());
  pending_jobs_.erase(it);
}

MockAsyncProxyResolver::MockAsyncProxyResolver() {
}

MockAsyncProxyResolverFactory::Request::Request(
    MockAsyncProxyResolverFactory* factory,
    const scoped_refptr<ProxyResolverScriptData>& script_data,
    std::unique_ptr<ProxyResolver>* resolver,
    const CompletionCallback& callback)
    : factory_(factory),
      script_data_(script_data),
      resolver_(resolver),
      callback_(callback) {}

MockAsyncProxyResolverFactory::Request::~Request() {
}

void MockAsyncProxyResolverFactory::Request::CompleteNow(
    int rv,
    std::unique_ptr<ProxyResolver> resolver) {
  *resolver_ = std::move(resolver);

  // RemovePendingRequest may remove the last external reference to |this|.
  scoped_refptr<MockAsyncProxyResolverFactory::Request> keep_alive(this);
  factory_->RemovePendingRequest(this);
  factory_ = nullptr;
  callback_.Run(rv);
}

void MockAsyncProxyResolverFactory::Request::CompleteNowWithForwarder(
    int rv,
    ProxyResolver* resolver) {
  DCHECK(resolver);
  CompleteNow(rv, std::make_unique<ForwardingProxyResolver>(resolver));
}

void MockAsyncProxyResolverFactory::Request::FactoryDestroyed() {
  factory_ = nullptr;
}

class MockAsyncProxyResolverFactory::Job
    : public ProxyResolverFactory::Request {
 public:
  explicit Job(
      const scoped_refptr<MockAsyncProxyResolverFactory::Request>& request)
      : request_(request) {}
  ~Job() override {
    if (request_->factory_) {
      request_->factory_->cancelled_requests_.push_back(request_);
      request_->factory_->RemovePendingRequest(request_.get());
    }
  }

 private:
  scoped_refptr<MockAsyncProxyResolverFactory::Request> request_;
};

MockAsyncProxyResolverFactory::MockAsyncProxyResolverFactory(
    bool resolvers_expect_pac_bytes)
    : ProxyResolverFactory(resolvers_expect_pac_bytes) {
}

int MockAsyncProxyResolverFactory::CreateProxyResolver(
    const scoped_refptr<ProxyResolverScriptData>& pac_script,
    std::unique_ptr<ProxyResolver>* resolver,
    const net::CompletionCallback& callback,
    std::unique_ptr<ProxyResolverFactory::Request>* request_handle) {
  scoped_refptr<Request> request =
      new Request(this, pac_script, resolver, callback);
  pending_requests_.push_back(request);

  request_handle->reset(new Job(request));

  // Test code completes the request by calling request->CompleteNow().
  return ERR_IO_PENDING;
}

void MockAsyncProxyResolverFactory::RemovePendingRequest(Request* request) {
  RequestsList::iterator it =
      std::find(pending_requests_.begin(), pending_requests_.end(), request);
  DCHECK(it != pending_requests_.end());
  pending_requests_.erase(it);
}

MockAsyncProxyResolverFactory::~MockAsyncProxyResolverFactory() {
  for (auto& request : pending_requests_) {
    request->FactoryDestroyed();
  }
}

ForwardingProxyResolver::ForwardingProxyResolver(ProxyResolver* impl)
    : impl_(impl) {
}

int ForwardingProxyResolver::GetProxyForURL(const GURL& query_url,
                                            ProxyInfo* results,
                                            const CompletionCallback& callback,
                                            std::unique_ptr<Request>* request,
                                            const NetLogWithSource& net_log) {
  return impl_->GetProxyForURL(query_url, results, callback, request, net_log);
}

}  // namespace net
