// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/mojo/device_impl.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "device/usb/mojo/type_converters.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device.h"
#include "net/base/io_buffer.h"

namespace device {

using mojom::UsbControlTransferParamsPtr;
using mojom::UsbControlTransferRecipient;

namespace usb {

namespace {

scoped_refptr<net::IOBuffer> CreateTransferBuffer(size_t size) {
  return new net::IOBuffer(
      std::max(static_cast<size_t>(1u), static_cast<size_t>(size)));
}

void OnTransferIn(mojom::UsbDevice::GenericTransferInCallback callback,
                  UsbTransferStatus status,
                  scoped_refptr<net::IOBuffer> buffer,
                  size_t buffer_size) {
  std::vector<uint8_t> data;
  if (buffer) {
    // TODO(rockot/reillyg): We should change UsbDeviceHandle to use a
    // std::vector<uint8_t> instead of net::IOBuffer. Then we could move
    // instead of copy.
    data.resize(buffer_size);
    std::copy(buffer->data(), buffer->data() + buffer_size, data.begin());
  }

  std::move(callback).Run(mojo::ConvertTo<mojom::UsbTransferStatus>(status),
                          data);
}

void OnTransferOut(mojom::UsbDevice::GenericTransferOutCallback callback,
                   UsbTransferStatus status,
                   scoped_refptr<net::IOBuffer> buffer,
                   size_t buffer_size) {
  std::move(callback).Run(mojo::ConvertTo<mojom::UsbTransferStatus>(status));
}

std::vector<mojom::UsbIsochronousPacketPtr> BuildIsochronousPacketArray(
    const std::vector<uint32_t>& packet_lengths,
    mojom::UsbTransferStatus status) {
  std::vector<mojom::UsbIsochronousPacketPtr> packets;
  packets.reserve(packet_lengths.size());
  for (uint32_t packet_length : packet_lengths) {
    auto packet = mojom::UsbIsochronousPacket::New();
    packet->length = packet_length;
    packet->status = status;
    packets.push_back(std::move(packet));
  }
  return packets;
}

void OnIsochronousTransferIn(
    mojom::UsbDevice::IsochronousTransferInCallback callback,
    scoped_refptr<net::IOBuffer> buffer,
    const std::vector<UsbDeviceHandle::IsochronousPacket>& packets) {
  std::vector<uint8_t> data;
  if (buffer) {
    // TODO(rockot/reillyg): We should change UsbDeviceHandle to use a
    // std::vector<uint8_t> instead of net::IOBuffer. Then we could move
    // instead of copy.
    uint32_t buffer_size =
        std::accumulate(packets.begin(), packets.end(), 0u,
                        [](const uint32_t& a,
                           const UsbDeviceHandle::IsochronousPacket& packet) {
                          return a + packet.length;
                        });
    data.resize(buffer_size);
    std::copy(buffer->data(), buffer->data() + buffer_size, data.begin());
  }
  std::move(callback).Run(
      data,
      mojo::ConvertTo<std::vector<mojom::UsbIsochronousPacketPtr>>(packets));
}

void OnIsochronousTransferOut(
    mojom::UsbDevice::IsochronousTransferOutCallback callback,
    scoped_refptr<net::IOBuffer> buffer,
    const std::vector<UsbDeviceHandle::IsochronousPacket>& packets) {
  std::move(callback).Run(
      mojo::ConvertTo<std::vector<mojom::UsbIsochronousPacketPtr>>(packets));
}

}  // namespace

// static
void DeviceImpl::Create(scoped_refptr<device::UsbDevice> device,
                        base::WeakPtr<PermissionProvider> permission_provider,
                        mojom::UsbDeviceRequest request) {
  auto* device_impl =
      new DeviceImpl(std::move(device), std::move(permission_provider));
  device_impl->binding_ = mojo::MakeStrongBinding(base::WrapUnique(device_impl),
                                                  std::move(request));
}

DeviceImpl::~DeviceImpl() {
  CloseHandle();
}

DeviceImpl::DeviceImpl(scoped_refptr<device::UsbDevice> device,
                       base::WeakPtr<PermissionProvider> permission_provider)
    : device_(std::move(device)),
      permission_provider_(std::move(permission_provider)),
      observer_(this),
      weak_factory_(this) {
  DCHECK(device_);
  observer_.Add(device_.get());
}

void DeviceImpl::CloseHandle() {
  if (device_handle_) {
    device_handle_->Close();
    if (permission_provider_)
      permission_provider_->DecrementConnectionCount();
  }
  device_handle_ = nullptr;
}

bool DeviceImpl::HasControlTransferPermission(
    UsbControlTransferRecipient recipient,
    uint16_t index) {
  DCHECK(device_handle_);

  if (recipient != UsbControlTransferRecipient::INTERFACE &&
      recipient != UsbControlTransferRecipient::ENDPOINT) {
    return true;
  }

  const UsbConfigDescriptor* config = device_->active_configuration();
  if (!config)
    return false;

  const UsbInterfaceDescriptor* interface = nullptr;
  if (recipient == UsbControlTransferRecipient::ENDPOINT) {
    interface = device_handle_->FindInterfaceByEndpoint(index & 0xff);
  } else {
    auto interface_it =
        std::find_if(config->interfaces.begin(), config->interfaces.end(),
                     [index](const UsbInterfaceDescriptor& this_iface) {
                       return this_iface.interface_number == (index & 0xff);
                     });
    if (interface_it != config->interfaces.end())
      interface = &*interface_it;
  }

  return interface != nullptr;
}

// static
void DeviceImpl::OnOpen(base::WeakPtr<DeviceImpl> self,
                        OpenCallback callback,
                        scoped_refptr<UsbDeviceHandle> handle) {
  if (!self) {
    handle->Close();
    return;
  }

  self->device_handle_ = std::move(handle);
  if (self->device_handle_ && self->permission_provider_)
    self->permission_provider_->IncrementConnectionCount();
  std::move(callback).Run(self->device_handle_
                              ? mojom::UsbOpenDeviceError::OK
                              : mojom::UsbOpenDeviceError::ACCESS_DENIED);
}

void DeviceImpl::OnPermissionGrantedForOpen(OpenCallback callback,
                                            bool granted) {
  if (granted) {
    device_->Open(base::BindOnce(
        &DeviceImpl::OnOpen, weak_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    std::move(callback).Run(mojom::UsbOpenDeviceError::ACCESS_DENIED);
  }
}

void DeviceImpl::Open(OpenCallback callback) {
  if (device_handle_) {
    std::move(callback).Run(mojom::UsbOpenDeviceError::ALREADY_OPEN);
    return;
  }

  if (!device_->permission_granted()) {
    device_->RequestPermission(
        base::BindOnce(&DeviceImpl::OnPermissionGrantedForOpen,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  device_->Open(base::BindOnce(&DeviceImpl::OnOpen, weak_factory_.GetWeakPtr(),
                               std::move(callback)));
}

void DeviceImpl::Close(CloseCallback callback) {
  CloseHandle();
  std::move(callback).Run();
}

void DeviceImpl::SetConfiguration(uint8_t value,
                                  SetConfigurationCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->SetConfiguration(value, std::move(callback));
}

void DeviceImpl::ClaimInterface(uint8_t interface_number,
                                ClaimInterfaceCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  const UsbConfigDescriptor* config = device_->active_configuration();
  if (!config) {
    std::move(callback).Run(false);
    return;
  }

  auto interface_it =
      std::find_if(config->interfaces.begin(), config->interfaces.end(),
                   [interface_number](const UsbInterfaceDescriptor& interface) {
                     return interface.interface_number == interface_number;
                   });
  if (interface_it == config->interfaces.end()) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->ClaimInterface(interface_number, std::move(callback));
}

void DeviceImpl::ReleaseInterface(uint8_t interface_number,
                                  ReleaseInterfaceCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->ReleaseInterface(interface_number, std::move(callback));
}

void DeviceImpl::SetInterfaceAlternateSetting(
    uint8_t interface_number,
    uint8_t alternate_setting,
    SetInterfaceAlternateSettingCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->SetInterfaceAlternateSetting(
      interface_number, alternate_setting, std::move(callback));
}

void DeviceImpl::Reset(ResetCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->ResetDevice(std::move(callback));
}

void DeviceImpl::ClearHalt(uint8_t endpoint, ClearHaltCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(false);
    return;
  }

  device_handle_->ClearHalt(endpoint, std::move(callback));
}

void DeviceImpl::ControlTransferIn(UsbControlTransferParamsPtr params,
                                   uint32_t length,
                                   uint32_t timeout,
                                   ControlTransferInCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(mojom::UsbTransferStatus::TRANSFER_ERROR,
                            base::nullopt);
    return;
  }

  if (HasControlTransferPermission(params->recipient, params->index)) {
    scoped_refptr<net::IOBuffer> buffer = CreateTransferBuffer(length);
    device_handle_->ControlTransfer(
        UsbTransferDirection::INBOUND, params->type, params->recipient,
        params->request, params->value, params->index, buffer, length, timeout,
        base::BindOnce(&OnTransferIn, std::move(callback)));
  } else {
    std::move(callback).Run(mojom::UsbTransferStatus::PERMISSION_DENIED,
                            base::nullopt);
  }
}

void DeviceImpl::ControlTransferOut(UsbControlTransferParamsPtr params,
                                    const std::vector<uint8_t>& data,
                                    uint32_t timeout,
                                    ControlTransferOutCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(mojom::UsbTransferStatus::TRANSFER_ERROR);
    return;
  }

  if (HasControlTransferPermission(params->recipient, params->index)) {
    scoped_refptr<net::IOBuffer> buffer = CreateTransferBuffer(data.size());
    std::copy(data.begin(), data.end(), buffer->data());
    device_handle_->ControlTransfer(
        UsbTransferDirection::OUTBOUND, params->type, params->recipient,
        params->request, params->value, params->index, buffer, data.size(),
        timeout, base::BindOnce(&OnTransferOut, std::move(callback)));
  } else {
    std::move(callback).Run(mojom::UsbTransferStatus::PERMISSION_DENIED);
  }
}

void DeviceImpl::GenericTransferIn(uint8_t endpoint_number,
                                   uint32_t length,
                                   uint32_t timeout,
                                   GenericTransferInCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(mojom::UsbTransferStatus::TRANSFER_ERROR,
                            base::nullopt);
    return;
  }

  uint8_t endpoint_address = endpoint_number | 0x80;
  scoped_refptr<net::IOBuffer> buffer = CreateTransferBuffer(length);
  device_handle_->GenericTransfer(
      UsbTransferDirection::INBOUND, endpoint_address, buffer, length, timeout,
      base::BindOnce(&OnTransferIn, std::move(callback)));
}

void DeviceImpl::GenericTransferOut(uint8_t endpoint_number,
                                    const std::vector<uint8_t>& data,
                                    uint32_t timeout,
                                    GenericTransferOutCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(mojom::UsbTransferStatus::TRANSFER_ERROR);
    return;
  }

  uint8_t endpoint_address = endpoint_number;
  scoped_refptr<net::IOBuffer> buffer = CreateTransferBuffer(data.size());
  std::copy(data.begin(), data.end(), buffer->data());
  device_handle_->GenericTransfer(
      UsbTransferDirection::OUTBOUND, endpoint_address, buffer, data.size(),
      timeout, base::BindOnce(&OnTransferOut, std::move(callback)));
}

void DeviceImpl::IsochronousTransferIn(
    uint8_t endpoint_number,
    const std::vector<uint32_t>& packet_lengths,
    uint32_t timeout,
    IsochronousTransferInCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(
        base::nullopt,
        BuildIsochronousPacketArray(packet_lengths,
                                    mojom::UsbTransferStatus::TRANSFER_ERROR));
    return;
  }

  uint8_t endpoint_address = endpoint_number | 0x80;
  device_handle_->IsochronousTransferIn(
      endpoint_address, packet_lengths, timeout,
      base::BindOnce(&OnIsochronousTransferIn, std::move(callback)));
}

void DeviceImpl::IsochronousTransferOut(
    uint8_t endpoint_number,
    const std::vector<uint8_t>& data,
    const std::vector<uint32_t>& packet_lengths,
    uint32_t timeout,
    IsochronousTransferOutCallback callback) {
  if (!device_handle_) {
    std::move(callback).Run(BuildIsochronousPacketArray(
        packet_lengths, mojom::UsbTransferStatus::TRANSFER_ERROR));
    return;
  }

  uint8_t endpoint_address = endpoint_number;
  scoped_refptr<net::IOBuffer> buffer = CreateTransferBuffer(data.size());
  std::copy(data.begin(), data.end(), buffer->data());
  device_handle_->IsochronousTransferOut(
      endpoint_address, buffer, packet_lengths, timeout,
      base::BindOnce(&OnIsochronousTransferOut, std::move(callback)));
}

void DeviceImpl::OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) {
  DCHECK_EQ(device_, device);
  binding_->Close();
}

}  // namespace usb
}  // namespace device
