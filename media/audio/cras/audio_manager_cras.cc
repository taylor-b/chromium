// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/cras/audio_manager_cras.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <utility>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/logging.h"
#include "base/nix/xdg_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/audio/audio_device.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_features.h"
#include "media/audio/cras/cras_input.h"
#include "media/audio/cras/cras_unified.h"
#include "media/base/channel_layout.h"
#include "media/base/limits.h"
#include "media/base/localized_strings.h"

// cras_util.h headers pull in min/max macros...
// TODO(dgreid): Fix headers such that these aren't imported.
#undef min
#undef max

namespace media {
namespace {

// Maximum number of output streams that can be open simultaneously.
const int kMaxOutputStreams = 50;

// Default sample rate for input and output streams.
const int kDefaultSampleRate = 48000;

// Default input buffer size.
const int kDefaultInputBufferSize = 1024;

const char kInternalInputVirtualDevice[] = "Built-in mic";
const char kInternalOutputVirtualDevice[] = "Built-in speaker";
const char kHeadphoneLineOutVirtualDevice[] = "Headphone/Line Out";

// Used for the Media.CrosBeamformingDeviceState histogram, currently not used
// since beamforming is disabled.
enum CrosBeamformingDeviceState {
  BEAMFORMING_DEFAULT_ENABLED = 0,
  BEAMFORMING_USER_ENABLED,
  BEAMFORMING_DEFAULT_DISABLED,
  BEAMFORMING_USER_DISABLED,
  BEAMFORMING_STATE_MAX = BEAMFORMING_USER_DISABLED
};

bool HasKeyboardMic(const chromeos::AudioDeviceList& devices) {
  for (const auto& device : devices) {
    if (device.is_input && device.type == chromeos::AUDIO_TYPE_KEYBOARD_MIC) {
      return true;
    }
  }
  return false;
}

const chromeos::AudioDevice* GetDeviceFromId(
    const chromeos::AudioDeviceList& devices,
    uint64_t id) {
  for (const auto& device : devices) {
    if (device.id == id) {
      return &device;
    }
  }
  return nullptr;
}

// Process |device_list| that two shares the same dev_index by creating a
// virtual device name for them.
void ProcessVirtualDeviceName(AudioDeviceNames* device_names,
                              const chromeos::AudioDeviceList& device_list) {
  DCHECK_EQ(2U, device_list.size());
  if (device_list[0].type == chromeos::AUDIO_TYPE_LINEOUT ||
      device_list[1].type == chromeos::AUDIO_TYPE_LINEOUT) {
    device_names->emplace_back(kHeadphoneLineOutVirtualDevice,
                               base::Uint64ToString(device_list[0].id));
  } else if (device_list[0].type == chromeos::AUDIO_TYPE_INTERNAL_SPEAKER ||
             device_list[1].type == chromeos::AUDIO_TYPE_INTERNAL_SPEAKER) {
    device_names->emplace_back(kInternalOutputVirtualDevice,
                               base::Uint64ToString(device_list[0].id));
  } else {
    DCHECK(device_list[0].type == chromeos::AUDIO_TYPE_INTERNAL_MIC ||
           device_list[1].type == chromeos::AUDIO_TYPE_INTERNAL_MIC);
    device_names->emplace_back(kInternalInputVirtualDevice,
                               base::Uint64ToString(device_list[0].id));
  }
}

}  // namespace

bool AudioManagerCras::HasAudioOutputDevices() {
  return true;
}

bool AudioManagerCras::HasAudioInputDevices() {
  chromeos::AudioDeviceList devices;
  GetAudioDevices(&devices);
  for (size_t i = 0; i < devices.size(); ++i) {
    if (devices[i].is_input && devices[i].is_for_simple_usage())
      return true;
  }
  return false;
}

AudioManagerCras::AudioManagerCras(std::unique_ptr<AudioThread> audio_thread,
                                   AudioLogFactory* audio_log_factory)
    : AudioManagerBase(std::move(audio_thread), audio_log_factory),
      on_shutdown_(base::WaitableEvent::ResetPolicy::MANUAL,
                   base::WaitableEvent::InitialState::NOT_SIGNALED),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
  SetMaxOutputStreamsAllowed(kMaxOutputStreams);
}

AudioManagerCras::~AudioManagerCras() = default;

void AudioManagerCras::GetAudioDeviceNamesImpl(bool is_input,
                                               AudioDeviceNames* device_names) {
  DCHECK(device_names->empty());

  device_names->push_back(AudioDeviceName::CreateDefault());

  if (base::FeatureList::IsEnabled(features::kEnumerateAudioDevices)) {
    chromeos::AudioDeviceList devices;
    GetAudioDevices(&devices);

    // |dev_idx_map| is a map of dev_index and their audio devices.
    std::map<int, chromeos::AudioDeviceList> dev_idx_map;
    for (const auto& device : devices) {
      if (device.is_input != is_input || !device.is_for_simple_usage())
        continue;

      dev_idx_map[dev_index_of(device.id)].push_back(device);
    }

    for (const auto& item : dev_idx_map) {
      if (1 == item.second.size()) {
        const chromeos::AudioDevice& device = item.second.front();
        device_names->emplace_back(device.display_name,
                                   base::Uint64ToString(device.id));
      } else {
        // Create virtual device name for audio nodes that share the same device
        // index.
        ProcessVirtualDeviceName(device_names, item.second);
      }
    }
  }
}

void AudioManagerCras::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  GetAudioDeviceNamesImpl(true, device_names);
}

void AudioManagerCras::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  GetAudioDeviceNamesImpl(false, device_names);
}

AudioParameters AudioManagerCras::GetInputStreamParameters(
    const std::string& device_id) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());

  int user_buffer_size = GetUserBufferSize();
  int buffer_size = user_buffer_size ?
      user_buffer_size : kDefaultInputBufferSize;

  // TODO(hshi): Fine-tune audio parameters based on |device_id|. The optimal
  // parameters for the loopback stream may differ from the default.
  AudioParameters params(AudioParameters::AUDIO_PCM_LOW_LATENCY,
                         CHANNEL_LAYOUT_STEREO, kDefaultSampleRate, 16,
                         buffer_size);
  chromeos::AudioDeviceList devices;
  GetAudioDevices(&devices);
  if (HasKeyboardMic(devices))
    params.set_effects(AudioParameters::KEYBOARD_MIC);

  return params;
}

std::string AudioManagerCras::GetAssociatedOutputDeviceID(
    const std::string& input_device_id) {
  if (!base::FeatureList::IsEnabled(features::kEnumerateAudioDevices))
    return "";

  chromeos::AudioDeviceList devices;
  GetAudioDevices(&devices);

  // At this point, we know we have an ordinary input device, so we look up its
  // device_name, which identifies which hardware device it belongs to.
  uint64_t device_id = 0;
  if (!base::StringToUint64(input_device_id, &device_id))
    return "";
  const chromeos::AudioDevice* input_device =
      GetDeviceFromId(devices, device_id);
  if (!input_device)
    return "";

  const base::StringPiece device_name = input_device->device_name;

  // Now search for an output device with the same device name.
  auto output_device_it = std::find_if(
      devices.begin(), devices.end(),
      [device_name](const chromeos::AudioDevice& device) {
        return !device.is_input && device.device_name == device_name;
      });
  return output_device_it == devices.end()
             ? ""
             : base::Uint64ToString(output_device_it->id);
}

std::string AudioManagerCras::GetDefaultOutputDeviceID() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  uint64_t active_output_node_id = 0;
  if (main_task_runner_->BelongsToCurrentThread()) {
    // Unittest may use the same thread for audio thread.
    GetPrimaryActiveOutputNodeOnMainThread(&active_output_node_id, &event);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &AudioManagerCras::GetPrimaryActiveOutputNodeOnMainThread,
            weak_this_, base::Unretained(&active_output_node_id),
            base::Unretained(&event)));
  }
  WaitEventOrShutdown(&event);
  return base::Uint64ToString(active_output_node_id);
}

const char* AudioManagerCras::GetName() {
  return "CRAS";
}

bool AudioManagerCras::Shutdown() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  weak_ptr_factory_.InvalidateWeakPtrs();
  on_shutdown_.Signal();
  return AudioManager::Shutdown();
}

AudioOutputStream* AudioManagerCras::MakeLinearOutputStream(
    const AudioParameters& params,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  // Pinning stream is not supported for MakeLinearOutputStream.
  return MakeOutputStream(params, AudioDeviceDescription::kDefaultDeviceId);
}

AudioOutputStream* AudioManagerCras::MakeLowLatencyOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  // TODO(dgreid): Open the correct input device for unified IO.
  return MakeOutputStream(params, device_id);
}

AudioInputStream* AudioManagerCras::MakeLinearInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return MakeInputStream(params, device_id);
}

AudioInputStream* AudioManagerCras::MakeLowLatencyInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  return MakeInputStream(params, device_id);
}

int AudioManagerCras::GetDefaultOutputBufferSizePerBoard() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  int32_t buffer_size = 512;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  if (main_task_runner_->BelongsToCurrentThread()) {
    // Unittest may use the same thread for audio thread.
    GetDefaultOutputBufferSizeOnMainThread(&buffer_size, &event);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &AudioManagerCras::GetDefaultOutputBufferSizeOnMainThread,
            weak_this_, base::Unretained(&buffer_size),
            base::Unretained(&event)));
  }
  WaitEventOrShutdown(&event);
  return static_cast<int>(buffer_size);
}

AudioParameters AudioManagerCras::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  ChannelLayout channel_layout = CHANNEL_LAYOUT_STEREO;
  int sample_rate = kDefaultSampleRate;
  int buffer_size = GetDefaultOutputBufferSizePerBoard();
  int bits_per_sample = 16;
  if (input_params.IsValid()) {
    sample_rate = input_params.sample_rate();
    bits_per_sample = input_params.bits_per_sample();
    channel_layout = input_params.channel_layout();
    buffer_size =
        std::min(static_cast<int>(limits::kMaxAudioBufferSize),
                 std::max(static_cast<int>(limits::kMinAudioBufferSize),
                          input_params.frames_per_buffer()));
  }

  int user_buffer_size = GetUserBufferSize();
  if (user_buffer_size)
    buffer_size = user_buffer_size;

  return AudioParameters(AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
                         sample_rate, bits_per_sample, buffer_size);
}

AudioOutputStream* AudioManagerCras::MakeOutputStream(
    const AudioParameters& params,
    const std::string& device_id) {
  return new CrasUnifiedStream(params, this, device_id);
}

AudioInputStream* AudioManagerCras::MakeInputStream(
    const AudioParameters& params, const std::string& device_id) {
  return new CrasInputStream(params, this, device_id);
}

snd_pcm_format_t AudioManagerCras::BitsToFormat(int bits_per_sample) {
  switch (bits_per_sample) {
    case 8:
      return SND_PCM_FORMAT_U8;
    case 16:
      return SND_PCM_FORMAT_S16;
    case 24:
      return SND_PCM_FORMAT_S24;
    case 32:
      return SND_PCM_FORMAT_S32;
    default:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

bool AudioManagerCras::IsDefault(const std::string& device_id, bool is_input) {
  AudioDeviceNames device_names;
  GetAudioDeviceNamesImpl(is_input, &device_names);
  DCHECK(!device_names.empty());
  const AudioDeviceName& device_name = device_names.front();
  return device_name.unique_id == device_id;
}

void AudioManagerCras::GetAudioDevices(chromeos::AudioDeviceList* devices) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  if (main_task_runner_->BelongsToCurrentThread()) {
    GetAudioDevicesOnMainThread(devices, &event);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AudioManagerCras::GetAudioDevicesOnMainThread,
                       weak_this_, base::Unretained(devices),
                       base::Unretained(&event)));
  }
  WaitEventOrShutdown(&event);
}

void AudioManagerCras::GetAudioDevicesOnMainThread(
    chromeos::AudioDeviceList* devices,
    base::WaitableEvent* event) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // CrasAudioHandler is shut down before AudioManagerCras.
  if (chromeos::CrasAudioHandler::IsInitialized()) {
    chromeos::CrasAudioHandler::Get()->GetAudioDevices(devices);
  }
  event->Signal();
}

void AudioManagerCras::GetPrimaryActiveOutputNodeOnMainThread(
    uint64_t* active_output_node_id,
    base::WaitableEvent* event) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (chromeos::CrasAudioHandler::IsInitialized()) {
    chromeos::CrasAudioHandler::Get()->GetPrimaryActiveOutputNode();
  }
  event->Signal();
}

void AudioManagerCras::GetDefaultOutputBufferSizeOnMainThread(
    int32_t* buffer_size,
    base::WaitableEvent* event) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (chromeos::CrasAudioHandler::IsInitialized()) {
    chromeos::CrasAudioHandler::Get()->GetDefaultOutputBufferSize(buffer_size);
  }
  event->Signal();
}

void AudioManagerCras::WaitEventOrShutdown(base::WaitableEvent* event) {
  base::WaitableEvent* waitables[] = {event, &on_shutdown_};
  base::WaitableEvent::WaitMany(waitables, arraysize(waitables));
}

}  // namespace media
