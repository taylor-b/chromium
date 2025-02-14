// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.notifications.channels;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.hasSize;
import static org.hamcrest.Matchers.is;

import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.browser.notifications.NotificationChannelStatus;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;
import org.chromium.chrome.browser.notifications.NotificationSettingsBridge;
import org.chromium.content.browser.test.NativeLibraryTestRule;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Instrumentation unit tests for SiteChannelsManager.
 *
 * They run against the real NotificationManager so we can be sure Android does what we expect.
 *
 * Note that channels are persistent by Android so even if a channel is deleted, if it is recreated
 * with the same id then the previous properties will be restored, including whether the channel was
 * blocked. Thus some of these tests use different channel ids to avoid this problem.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class SiteChannelsManagerTest {
    private SiteChannelsManager mSiteChannelsManager;
    @Rule
    public NativeLibraryTestRule mNativeLibraryTestRule = new NativeLibraryTestRule();

    @Before
    public void setUp() throws Exception {
        // Not initializing the browser process is safe because
        // UrlFormatter.formatUrlForSecurityDisplay() is stand-alone.
        mNativeLibraryTestRule.loadNativeLibraryNoBrowserProcess();

        Context mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        NotificationManagerProxy notificationManagerProxy = new NotificationManagerProxyImpl(
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE));
        clearExistingSiteChannels(notificationManagerProxy);
        mSiteChannelsManager = new SiteChannelsManager(notificationManagerProxy);
    }

    private static void clearExistingSiteChannels(
            NotificationManagerProxy notificationManagerProxy) {
        for (NotificationChannel channel : notificationManagerProxy.getNotificationChannels()) {
            if (channel.getId().startsWith(ChannelDefinitions.CHANNEL_ID_PREFIX_SITES)
                    || (channel.getGroup() != null
                               && channel.getGroup().equals(
                                          ChannelDefinitions.CHANNEL_GROUP_ID_SITES))) {
                notificationManagerProxy.deleteNotificationChannel(channel.getId());
            }
        }
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testCreateSiteChannel_enabled() throws Exception {
        mSiteChannelsManager.createSiteChannel("https://example-enabled.org", 62102180000L, true);
        assertThat(Arrays.asList(mSiteChannelsManager.getSiteChannels()), hasSize(1));
        NotificationSettingsBridge.SiteChannel channel = mSiteChannelsManager.getSiteChannels()[0];
        assertThat(channel.getOrigin(), is("https://example-enabled.org"));
        assertThat(channel.getStatus(), matchesChannelStatus(NotificationChannelStatus.ENABLED));
        assertThat(channel.getTimestamp(), is(62102180000L));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testCreateSiteChannel_stripsSchemaForChannelName() throws Exception {
        mSiteChannelsManager.createSiteChannel("http://127.0.0.1", 0L, true);
        mSiteChannelsManager.createSiteChannel("https://example.com", 0L, true);
        mSiteChannelsManager.createSiteChannel("ftp://127.0.0.1", 0L, true);
        List<String> channelNames = new ArrayList<>();
        for (NotificationSettingsBridge.SiteChannel siteChannel :
                mSiteChannelsManager.getSiteChannels()) {
            channelNames.add(siteChannel.toChannel().getName().toString());
        }
        assertThat(channelNames, containsInAnyOrder("ftp://127.0.0.1", "example.com", "127.0.0.1"));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testCreateSiteChannel_disabled() throws Exception {
        mSiteChannelsManager.createSiteChannel("https://example-blocked.org", 0L, false);
        assertThat(Arrays.asList(mSiteChannelsManager.getSiteChannels()), hasSize(1));
        NotificationSettingsBridge.SiteChannel channel = mSiteChannelsManager.getSiteChannels()[0];
        assertThat(channel.getOrigin(), is("https://example-blocked.org"));
        assertThat(channel.getStatus(), matchesChannelStatus(NotificationChannelStatus.BLOCKED));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testDeleteSiteChannel_channelExists() throws Exception {
        NotificationSettingsBridge.SiteChannel channel =
                mSiteChannelsManager.createSiteChannel("https://chromium.org", 0L, true);
        mSiteChannelsManager.deleteSiteChannel(channel.getId());
        assertThat(Arrays.asList(mSiteChannelsManager.getSiteChannels()), hasSize(0));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testDeleteSiteChannel_channelDoesNotExist() throws Exception {
        mSiteChannelsManager.createSiteChannel("https://chromium.org", 0L, true);
        mSiteChannelsManager.deleteSiteChannel("https://some-other-origin.org");
        assertThat(Arrays.asList(mSiteChannelsManager.getSiteChannels()), hasSize(1));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testGetChannelStatus_channelCreatedAsEnabled() throws Exception {
        NotificationSettingsBridge.SiteChannel channel =
                mSiteChannelsManager.createSiteChannel("https://example-enabled.org", 0L, true);
        assertThat(mSiteChannelsManager.getChannelStatus(channel.getId()),
                matchesChannelStatus(NotificationChannelStatus.ENABLED));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testGetChannelStatus_channelCreatedAsBlocked() throws Exception {
        assertThat(mSiteChannelsManager.getChannelStatus("https://example-blocked.com"),
                matchesChannelStatus(NotificationChannelStatus.UNAVAILABLE));
        NotificationSettingsBridge.SiteChannel channel =
                mSiteChannelsManager.createSiteChannel("https://example-blocked.com", 0L, false);
        assertThat(mSiteChannelsManager.getChannelStatus(channel.getId()),
                matchesChannelStatus(NotificationChannelStatus.BLOCKED));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testGetChannelStatus_channelNotCreated() throws Exception {
        assertThat(mSiteChannelsManager.getChannelStatus("invalid-channel-id"),
                matchesChannelStatus(NotificationChannelStatus.UNAVAILABLE));
    }

    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @SmallTest
    public void testGetChannelStatus_channelCreatedThenDeleted() throws Exception {
        NotificationSettingsBridge.SiteChannel channel =
                mSiteChannelsManager.createSiteChannel("https://chromium.org", 0L, true);
        mSiteChannelsManager.deleteSiteChannel(channel.getId());
        assertThat(mSiteChannelsManager.getChannelStatus(channel.getId()),
                matchesChannelStatus(NotificationChannelStatus.UNAVAILABLE));
    }

    private static Matcher<Integer> matchesChannelStatus(
            @NotificationChannelStatus final int status) {
        return new BaseMatcher<Integer>() {
            @Override
            public boolean matches(Object o) {
                return status == (int) o;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("NotificationChannelStatus." + statusToString(status));
            }

            @Override
            public void describeMismatch(final Object item, final Description description) {
                description.appendText(
                        "was NotificationChannelStatus." + statusToString((int) item));
            }
        };
    }

    private static String statusToString(@NotificationChannelStatus int status) {
        switch (status) {
            case NotificationChannelStatus.ENABLED:
                return "ENABLED";
            case NotificationChannelStatus.BLOCKED:
                return "BLOCKED";
            case NotificationChannelStatus.UNAVAILABLE:
                return "UNAVAILABLE";
        }
        return null;
    }
}