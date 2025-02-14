// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/strings/sys_string_conversions.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_info.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#import "ios/chrome/browser/ui/authentication/signin_earlgrey_utils.h"
#import "ios/chrome/browser/ui/settings/settings_collection_view_controller.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity_service.h"

#include "components/signin/core/browser/signin_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::PrimarySignInButton;
using chrome_test_util::SecondarySignInButton;
using chrome_test_util::SettingsAccountButton;
using chrome_test_util::ButtonWithAccessibilityLabelId;

namespace {

id<GREYMatcher> NavigationBarDoneButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_NAVIGATION_BAR_DONE_BUTTON);
}
}

@interface SigninSettingsTestCase : ChromeTestCase
@end

@implementation SigninSettingsTestCase

// Tests the primary button with a cold state.
- (void)testSignInPromoWithColdStateUsingPrimaryButton {
  [ChromeEarlGreyUI openSettingsMenu];
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeColdState];
  [ChromeEarlGreyUI tapSettingsMenuButton:PrimarySignInButton()];

  [[EarlGrey selectElementWithMatcher:grey_buttonTitle(@"Cancel")]
      performAction:grey_tap()];
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeColdState];
}

// Tests signing in, using the primary button with a warm state.
- (void)testSignInPromoWithWarmStateUsingPrimaryButton {
  ChromeIdentity* identity = [SigninEarlGreyUtils fakeIdentity1];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      identity);

  [ChromeEarlGreyUI openSettingsMenu];
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState];
  [ChromeEarlGreyUI tapSettingsMenuButton:PrimarySignInButton()];
  [ChromeEarlGreyUI confirmSigninConfirmationDialog];

  // User signed in.
  [SigninEarlGreyUtils assertSignedInWithIdentity:identity];
  [SigninEarlGreyUtils checkSigninPromoNotVisible];
  [[EarlGrey selectElementWithMatcher:SettingsAccountButton()]
      assertWithMatcher:grey_interactable()];
}

// Tests signing in, using the secondary button with a warm state.
- (void)testSignInPromoWithWarmStateUsingSecondaryButton {
  ChromeIdentity* identity = [SigninEarlGreyUtils fakeIdentity1];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      identity);

  [ChromeEarlGreyUI openSettingsMenu];
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState];
  [ChromeEarlGreyUI tapSettingsMenuButton:SecondarySignInButton()];
  [ChromeEarlGreyUI signInToIdentityByEmail:identity.userEmail];
  [ChromeEarlGreyUI confirmSigninConfirmationDialog];

  // User signed in.
  [SigninEarlGreyUtils assertSignedInWithIdentity:identity];
  [SigninEarlGreyUtils checkSigninPromoNotVisible];
  [[EarlGrey selectElementWithMatcher:SettingsAccountButton()]
      assertWithMatcher:grey_interactable()];
}

// Tests that the sign-in promo should not be shown after been shown 5 times.
- (void)testAutomaticSigninPromoDismiss {
  const int displayedCount = 4;
  ios::ChromeBrowserState* browser_state =
      chrome_test_util::GetOriginalBrowserState();
  PrefService* prefs = browser_state->GetPrefs();
  prefs->SetInteger(prefs::kIosSettingsSigninPromoDisplayedCount,
                    displayedCount);
  [ChromeEarlGreyUI openSettingsMenu];
  // Check the sign-in promo view is visible.
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeColdState];
  // Check the sign-in promo will not be shown anymore.
  GREYAssertEqual(
      displayedCount + 1,
      prefs->GetInteger(prefs::kIosSettingsSigninPromoDisplayedCount),
      @"Should have incremented the display count");
  // Close the bookmark view and open it again.
  [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
      performAction:grey_tap()];
  [ChromeEarlGreyUI openSettingsMenu];
  // Check that the sign-in promo is not visible anymore.
  [SigninEarlGreyUtils checkSigninPromoNotVisible];
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   grey_accessibilityID(kSettingsSignInCellId),
                                   grey_sufficientlyVisible(), nil)]
      assertWithMatcher:grey_notNil()];
}

@end
