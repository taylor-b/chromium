// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_

#include <memory>
#include <string>

#include "base/threading/thread_checker.h"
#include "media/audio/audio_output_delegate.h"
#include "media/mojo/interfaces/audio_output_stream.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_audio_output_stream.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

// Provides a single AudioOutput, given the audio parameters to use.
class MEDIA_MOJO_EXPORT MojoAudioOutputStreamProvider
    : public mojom::AudioOutputStreamProvider {
 public:
  using CreateDelegateCallback =
      base::OnceCallback<std::unique_ptr<AudioOutputDelegate>(
          const AudioParameters& params,
          AudioOutputDelegate::EventHandler*)>;
  using DeleterCallback = base::OnceCallback<void(AudioOutputStreamProvider*)>;

  // |create_delegate_callback| is used to obtain an AudioOutputDelegate for the
  // AudioOutput when it's initialized and |deleter_callback| is called when
  // this class should be removed (stream ended/error). |deleter_callback| is
  // required to destroy |this| synchronously.
  MojoAudioOutputStreamProvider(mojom::AudioOutputStreamProviderRequest request,
                                CreateDelegateCallback create_delegate_callback,
                                DeleterCallback deleter_callback);

  ~MojoAudioOutputStreamProvider() override;

 private:
  // mojom::AudioOutputStreamProvider implementation.
  void Acquire(mojom::AudioOutputStreamRequest stream_request,
               const AudioParameters& params,
               AcquireCallback acquire_callback) override;

  // Called when |audio_output_| had an error.
  void OnError();

  // The callback for the Acquire() must be stored until the response is ready.
  AcquireCallback acquire_callback_;

  base::Optional<MojoAudioOutputStream> audio_output_;
  mojo::Binding<AudioOutputStreamProvider> binding_;
  CreateDelegateCallback create_delegate_callback_;
  DeleterCallback deleter_callback_;
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioOutputStreamProvider);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_
