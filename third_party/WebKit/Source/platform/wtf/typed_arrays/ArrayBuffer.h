/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ArrayBuffer_h
#define ArrayBuffer_h

#include "base/allocator/partition_allocator/oom.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/WTFExport.h"
#include "platform/wtf/typed_arrays/ArrayBufferContents.h"

namespace WTF {

class ArrayBuffer;
class ArrayBufferView;

class WTF_EXPORT ArrayBuffer : public RefCounted<ArrayBuffer> {
 public:
  static inline RefPtr<ArrayBuffer> Create(unsigned num_elements,
                                           unsigned element_byte_size);
  static inline RefPtr<ArrayBuffer> Create(ArrayBuffer*);
  static inline RefPtr<ArrayBuffer> Create(const void* source,
                                           unsigned byte_length);
  static inline RefPtr<ArrayBuffer> Create(ArrayBufferContents&);

  static inline RefPtr<ArrayBuffer> CreateOrNull(unsigned num_elements,
                                                 unsigned element_byte_size);

  // Only for use by DOMArrayBuffer::CreateUninitializedOrNull().
  static inline RefPtr<ArrayBuffer> CreateUninitializedOrNull(
      unsigned num_elements,
      unsigned element_byte_size);

  static inline RefPtr<ArrayBuffer> CreateShared(unsigned num_elements,
                                                 unsigned element_byte_size);
  static inline RefPtr<ArrayBuffer> CreateShared(const void* source,
                                                 unsigned byte_length);

  inline void* Data();
  inline const void* Data() const;
  inline void* DataShared();
  inline const void* DataShared() const;
  inline void* DataMaybeShared();
  inline const void* DataMaybeShared() const;
  inline unsigned ByteLength() const;

  // Creates a new ArrayBuffer object with copy of bytes in this object
  // ranging from |begin| upto but not including |end|.
  inline RefPtr<ArrayBuffer> Slice(int begin, int end) const;
  inline RefPtr<ArrayBuffer> Slice(int begin) const;

  void AddView(ArrayBufferView*);
  void RemoveView(ArrayBufferView*);

  bool Transfer(ArrayBufferContents&);
  bool ShareContentsWith(ArrayBufferContents&);
  bool IsNeutered() const { return is_neutered_; }
  bool IsShared() const { return contents_.IsShared(); }

  ~ArrayBuffer() {}

 protected:
  inline explicit ArrayBuffer(ArrayBufferContents&);

 private:
  static inline RefPtr<ArrayBuffer> Create(
      unsigned num_elements,
      unsigned element_byte_size,
      ArrayBufferContents::InitializationPolicy);
  static inline RefPtr<ArrayBuffer> CreateOrNull(
      unsigned num_elements,
      unsigned element_byte_size,
      ArrayBufferContents::InitializationPolicy);
  static inline RefPtr<ArrayBuffer> CreateShared(
      unsigned num_elements,
      unsigned element_byte_size,
      ArrayBufferContents::InitializationPolicy);

  inline RefPtr<ArrayBuffer> SliceImpl(unsigned begin, unsigned end) const;
  inline unsigned ClampIndex(int index) const;
  static inline int ClampValue(int x, int left, int right);

  ArrayBufferContents contents_;
  ArrayBufferView* first_view_;
  bool is_neutered_;
};

int ArrayBuffer::ClampValue(int x, int left, int right) {
  DCHECK_LE(left, right);
  if (x < left)
    x = left;
  if (right < x)
    x = right;
  return x;
}

RefPtr<ArrayBuffer> ArrayBuffer::Create(unsigned num_elements,
                                        unsigned element_byte_size) {
  return Create(num_elements, element_byte_size,
                ArrayBufferContents::kZeroInitialize);
}

RefPtr<ArrayBuffer> ArrayBuffer::Create(ArrayBuffer* other) {
  // TODO(binji): support creating a SharedArrayBuffer by copying another
  // ArrayBuffer?
  DCHECK(!other->IsShared());
  return ArrayBuffer::Create(other->Data(), other->ByteLength());
}

RefPtr<ArrayBuffer> ArrayBuffer::Create(const void* source,
                                        unsigned byte_length) {
  ArrayBufferContents contents(byte_length, 1, ArrayBufferContents::kNotShared,
                               ArrayBufferContents::kDontInitialize);
  if (UNLIKELY(!contents.Data()))
    OOM_CRASH();
  RefPtr<ArrayBuffer> buffer = AdoptRef(new ArrayBuffer(contents));
  memcpy(buffer->Data(), source, byte_length);
  return buffer;
}

RefPtr<ArrayBuffer> ArrayBuffer::Create(ArrayBufferContents& contents) {
  CHECK(contents.DataMaybeShared());
  return AdoptRef(new ArrayBuffer(contents));
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateOrNull(unsigned num_elements,
                                              unsigned element_byte_size) {
  return CreateOrNull(num_elements, element_byte_size,
                      ArrayBufferContents::kZeroInitialize);
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateUninitializedOrNull(
    unsigned num_elements,
    unsigned element_byte_size) {
  return CreateOrNull(num_elements, element_byte_size,
                      ArrayBufferContents::kDontInitialize);
}

RefPtr<ArrayBuffer> ArrayBuffer::Create(
    unsigned num_elements,
    unsigned element_byte_size,
    ArrayBufferContents::InitializationPolicy policy) {
  ArrayBufferContents contents(num_elements, element_byte_size,
                               ArrayBufferContents::kNotShared, policy);
  if (UNLIKELY(!contents.Data()))
    OOM_CRASH();
  return AdoptRef(new ArrayBuffer(contents));
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateOrNull(
    unsigned num_elements,
    unsigned element_byte_size,
    ArrayBufferContents::InitializationPolicy policy) {
  ArrayBufferContents contents(num_elements, element_byte_size,
                               ArrayBufferContents::kNotShared, policy);
  if (!contents.Data())
    return nullptr;
  return AdoptRef(new ArrayBuffer(contents));
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateShared(unsigned num_elements,
                                              unsigned element_byte_size) {
  return CreateShared(num_elements, element_byte_size,
                      ArrayBufferContents::kZeroInitialize);
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateShared(const void* source,
                                              unsigned byte_length) {
  ArrayBufferContents contents(byte_length, 1, ArrayBufferContents::kShared,
                               ArrayBufferContents::kDontInitialize);
  CHECK(contents.DataShared());
  RefPtr<ArrayBuffer> buffer = AdoptRef(new ArrayBuffer(contents));
  memcpy(buffer->DataShared(), source, byte_length);
  return buffer;
}

RefPtr<ArrayBuffer> ArrayBuffer::CreateShared(
    unsigned num_elements,
    unsigned element_byte_size,
    ArrayBufferContents::InitializationPolicy policy) {
  ArrayBufferContents contents(num_elements, element_byte_size,
                               ArrayBufferContents::kShared, policy);
  CHECK(contents.DataShared());
  return AdoptRef(new ArrayBuffer(contents));
}

ArrayBuffer::ArrayBuffer(ArrayBufferContents& contents)
    : first_view_(0), is_neutered_(false) {
  if (contents.IsShared())
    contents.ShareWith(contents_);
  else
    contents.Transfer(contents_);
}

void* ArrayBuffer::Data() {
  return contents_.Data();
}

const void* ArrayBuffer::Data() const {
  return contents_.Data();
}

void* ArrayBuffer::DataShared() {
  return contents_.DataShared();
}

const void* ArrayBuffer::DataShared() const {
  return contents_.DataShared();
}

void* ArrayBuffer::DataMaybeShared() {
  return contents_.DataMaybeShared();
}

const void* ArrayBuffer::DataMaybeShared() const {
  return contents_.DataMaybeShared();
}

unsigned ArrayBuffer::ByteLength() const {
  return contents_.SizeInBytes();
}

RefPtr<ArrayBuffer> ArrayBuffer::Slice(int begin, int end) const {
  return SliceImpl(ClampIndex(begin), ClampIndex(end));
}

RefPtr<ArrayBuffer> ArrayBuffer::Slice(int begin) const {
  return SliceImpl(ClampIndex(begin), ByteLength());
}

RefPtr<ArrayBuffer> ArrayBuffer::SliceImpl(unsigned begin, unsigned end) const {
  unsigned size = begin <= end ? end - begin : 0;
  return ArrayBuffer::Create(static_cast<const char*>(Data()) + begin, size);
}

unsigned ArrayBuffer::ClampIndex(int index) const {
  unsigned current_length = ByteLength();
  if (index < 0)
    index = current_length + index;
  return ClampValue(index, 0, current_length);
}

}  // namespace WTF

using WTF::ArrayBuffer;

#endif  // ArrayBuffer_h
