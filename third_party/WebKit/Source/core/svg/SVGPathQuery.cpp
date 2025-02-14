/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann
 * <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/svg/SVGPathQuery.h"

#include "core/svg/SVGPathByteStreamSource.h"
#include "core/svg/SVGPathConsumer.h"
#include "core/svg/SVGPathData.h"
#include "core/svg/SVGPathParser.h"
#include "platform/graphics/PathTraversalState.h"

namespace blink {

namespace {

class SVGPathTraversalState final : public SVGPathConsumer {
 public:
  SVGPathTraversalState(
      PathTraversalState::PathTraversalAction traversal_action,
      float desired_length = 0)
      : traversal_state_(traversal_action), segment_index_(0) {
    traversal_state_.desired_length_ = desired_length;
  }

  unsigned SegmentIndex() const { return segment_index_; }
  float TotalLength() const { return traversal_state_.total_length_; }
  FloatPoint ComputedPoint() const { return traversal_state_.current_; }

  bool ProcessSegment(bool has_more_data) {
    traversal_state_.ProcessSegment();
    if (traversal_state_.success_)
      return true;
    if (has_more_data)
      segment_index_++;
    return false;
  }

 private:
  void EmitSegment(const PathSegmentData&) override;

  PathTraversalState traversal_state_;
  unsigned segment_index_;
};

void SVGPathTraversalState::EmitSegment(const PathSegmentData& segment) {
  switch (segment.command) {
    case kPathSegMoveToAbs:
      traversal_state_.total_length_ +=
          traversal_state_.MoveTo(segment.target_point);
      break;
    case kPathSegLineToAbs:
      traversal_state_.total_length_ +=
          traversal_state_.LineTo(segment.target_point);
      break;
    case kPathSegClosePath:
      traversal_state_.total_length_ += traversal_state_.CloseSubpath();
      break;
    case kPathSegCurveToCubicAbs:
      traversal_state_.total_length_ += traversal_state_.CubicBezierTo(
          segment.point1, segment.point2, segment.target_point);
      break;
    default:
      NOTREACHED();
  }
}

void ExecuteQuery(const SVGPathByteStream& path_byte_stream,
                  SVGPathTraversalState& traversal_state) {
  SVGPathByteStreamSource source(path_byte_stream);
  SVGPathNormalizer normalizer(&traversal_state);

  bool has_more_data = source.HasMoreData();
  while (has_more_data) {
    PathSegmentData segment = source.ParseSegment();
    DCHECK_NE(segment.command, kPathSegUnknown);

    normalizer.EmitSegment(segment);

    has_more_data = source.HasMoreData();
    if (traversal_state.ProcessSegment(has_more_data))
      break;
  }
}

}  // namespace

SVGPathQuery::SVGPathQuery(const SVGPathByteStream& path_byte_stream)
    : path_byte_stream_(path_byte_stream) {}

float SVGPathQuery::GetTotalLength() const {
  SVGPathTraversalState traversal_state(
      PathTraversalState::kTraversalTotalLength);
  ExecuteQuery(path_byte_stream_, traversal_state);
  return traversal_state.TotalLength();
}

FloatPoint SVGPathQuery::GetPointAtLength(float length) const {
  SVGPathTraversalState traversal_state(
      PathTraversalState::kTraversalPointAtLength, length);
  ExecuteQuery(path_byte_stream_, traversal_state);
  return traversal_state.ComputedPoint();
}

}  // namespace blink
