// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.
#include "chrome/common/search/instant_types.h"
#include "chrome/common/search/ntp_logging_events.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/ntp_tiles/tile_visual_type.h"
#include "components/omnibox/common/omnibox_focus_state.h"
#include "ipc/ipc_message_macros.h"

IPC_ENUM_TRAITS_MAX_VALUE(OmniboxFocusState, OMNIBOX_FOCUS_STATE_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(OmniboxFocusChangeReason,
                          OMNIBOX_FOCUS_CHANGE_REASON_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(NTPLoggingEventType, NTP_EVENT_TYPE_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(ntp_tiles::TileSource, ntp_tiles::TileSource::LAST)

IPC_ENUM_TRAITS_MAX_VALUE(ntp_tiles::TileVisualType, ntp_tiles::TILE_TYPE_MAX)

IPC_STRUCT_TRAITS_BEGIN(InstantMostVisitedItem)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(title)
  IPC_STRUCT_TRAITS_MEMBER(thumbnail)
  IPC_STRUCT_TRAITS_MEMBER(favicon)
  IPC_STRUCT_TRAITS_MEMBER(source)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(ThemeBackgroundInfo)
  IPC_STRUCT_TRAITS_MEMBER(using_default_theme)
  IPC_STRUCT_TRAITS_MEMBER(background_color)
  IPC_STRUCT_TRAITS_MEMBER(text_color)
  IPC_STRUCT_TRAITS_MEMBER(link_color)
  IPC_STRUCT_TRAITS_MEMBER(text_color_light)
  IPC_STRUCT_TRAITS_MEMBER(header_color)
  IPC_STRUCT_TRAITS_MEMBER(section_border_color)
  IPC_STRUCT_TRAITS_MEMBER(theme_id)
  IPC_STRUCT_TRAITS_MEMBER(image_horizontal_alignment)
  IPC_STRUCT_TRAITS_MEMBER(image_vertical_alignment)
  IPC_STRUCT_TRAITS_MEMBER(image_tiling)
  IPC_STRUCT_TRAITS_MEMBER(image_height)
  IPC_STRUCT_TRAITS_MEMBER(has_attribution)
  IPC_STRUCT_TRAITS_MEMBER(logo_alternate)
IPC_STRUCT_TRAITS_END()
