// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXCLUSIVE_ACCESS_BUBBLE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_EXCLUSIVE_ACCESS_BUBBLE_VIEWS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_hide_callback.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/widget/widget_observer.h"

class ExclusiveAccessBubbleViewsContext;
class GURL;
namespace gfx {
class SlideAnimation;
}
namespace views {
class View;
class Widget;
}

class SubtleNotificationView;

// ExclusiveAccessBubbleViews is responsible for showing a bubble atop the
// screen in fullscreen/mouse lock mode, telling users how to exit and providing
// a click target. The bubble auto-hides, and re-shows when the user moves to
// the screen top.
class ExclusiveAccessBubbleViews : public ExclusiveAccessBubble,
                                   public content::NotificationObserver,
                                   public views::WidgetObserver,
                                   public views::LinkListener {
 public:
  ExclusiveAccessBubbleViews(
      ExclusiveAccessBubbleViewsContext* context,
      const GURL& url,
      ExclusiveAccessBubbleType bubble_type,
      ExclusiveAccessBubbleHideCallback bubble_first_hide_callback);
  ~ExclusiveAccessBubbleViews() override;

  void UpdateContent(
      const GURL& url,
      ExclusiveAccessBubbleType bubble_type,
      ExclusiveAccessBubbleHideCallback bubble_first_hide_callback);

  // Repositions |popup_| if it is visible.
  void RepositionIfVisible();

  views::View* GetView();

 private:
  // Starts or stops polling the mouse location based on |popup_| and
  // |bubble_type_|.
  void UpdateMouseWatcher();

  // Updates |popup|'s bounds given |animation_| and |animated_attribute_|.
  void UpdateBounds();

  void UpdateViewContent(ExclusiveAccessBubbleType bubble_type);

  // Returns the root view containing |browser_view_|.
  views::View* GetBrowserRootView() const;

  // ExclusiveAccessBubble:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;
  gfx::Rect GetPopupRect(bool ignore_animation_state) const override;
  gfx::Point GetCursorScreenPoint() override;
  bool WindowContainsPoint(gfx::Point pos) override;
  bool IsWindowActive() override;
  void Hide() override;
  void Show() override;
  bool IsAnimating() override;
  bool CanMouseTriggerSlideIn() const override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

  // views::LinkListener override:
  void LinkClicked(views::Link* source, int event_flags) override;

  ExclusiveAccessBubbleViewsContext* const bubble_view_context_;

  views::Widget* popup_;

  // Classic mode: Bubble may show & hide multiple times. The callback only runs
  // for the first hide.
  // Simplified mode: Bubble only hides once.
  ExclusiveAccessBubbleHideCallback bubble_first_hide_callback_;

  // Animation controlling showing/hiding of the exit bubble.
  std::unique_ptr<gfx::SlideAnimation> animation_;

  // The contents of the popup.
  SubtleNotificationView* view_;
  base::string16 browser_fullscreen_exit_accelerator_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExclusiveAccessBubbleViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXCLUSIVE_ACCESS_BUBBLE_VIEWS_H_
