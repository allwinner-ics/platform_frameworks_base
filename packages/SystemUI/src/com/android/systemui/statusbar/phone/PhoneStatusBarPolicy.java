/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.systemui.statusbar.phone;

import android.app.StatusBarManager;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.LocationManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Handler;
import android.os.RemoteException;
import android.os.storage.StorageManager;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.util.Slog;

import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.cdma.EriInfo;
import com.android.internal.telephony.cdma.TtyIntent;
import com.android.server.am.BatteryStatsService;
import com.android.systemui.R;
import android.view.DisplayManager;
import com.android.systemui.statusbar.DisplayHotPlugPolicy;

/**
 * This class contains all of the policy about which icons are installed in the status
 * bar at boot time.  It goes through the normal API for icons, even though it probably
 * strictly doesn't need to.
 */
public class PhoneStatusBarPolicy {
    private static final String TAG = "PhoneStatusBarPolicy";

    // message codes for the handler
    private static final int EVENT_BATTERY_CLOSE = 4;

    private static final int AM_PM_STYLE_NORMAL  = 0;
    private static final int AM_PM_STYLE_SMALL   = 1;
    private static final int AM_PM_STYLE_GONE    = 2;

    private static final int AM_PM_STYLE = AM_PM_STYLE_GONE;

    private static final int INET_CONDITION_THRESHOLD = 50;

    private static final boolean SHOW_SYNC_ICON = false;

    private final Context mContext;
    private final StatusBarManager mService;
    private final Handler mHandler = new Handler();

    // storage
    private StorageManager mStorageManager;


    // Assume it's all good unless we hear otherwise.  We don't always seem
    // to get broadcasts that it *is* there.
    IccCard.State mSimState = IccCard.State.READY;

    // ringer volume
    private boolean mVolumeVisible;

    // bluetooth device status
    private boolean mBluetoothEnabled;

    // wifi
    private static final int[][] sWifiSignalImages = {
            { R.drawable.stat_sys_wifi_signal_1,
              R.drawable.stat_sys_wifi_signal_2,
              R.drawable.stat_sys_wifi_signal_3,
              R.drawable.stat_sys_wifi_signal_4 },
            { R.drawable.stat_sys_wifi_signal_1_fully,
              R.drawable.stat_sys_wifi_signal_2_fully,
              R.drawable.stat_sys_wifi_signal_3_fully,
              R.drawable.stat_sys_wifi_signal_4_fully }
        };
    private static final int sWifiTemporarilyNotConnectedImage =
            R.drawable.stat_sys_wifi_signal_0;

    private int mLastWifiSignalLevel = -1;
    private boolean mIsWifiConnected = false;
	private static final boolean SHOW_HDMIPLUG_IN_CALL = true;
    private static final boolean SHOW_TVPLUG_IN_CALL = true;
    // state of inet connection - 0 not connected, 100 connected
    private int mInetCondition = 0;
	private final  DisplayManager mDisplayManager;
	private DisplayHotPlugPolicy  mDispHotPolicy = null;

    // sync state
    // If sync is active the SyncActive icon is displayed. If sync is not active but
    // sync is failing the SyncFailing icon is displayed. Otherwise neither are displayed.

    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
			Slog.d(TAG, "onReceive action = " + action);
			
            if (action.equals(Intent.ACTION_ALARM_CHANGED)) {
                updateAlarm(intent);
            }
			else if (action.equals(Intent.ACTION_HDMISTATUS_CHANGED)) {
                onHdmiPlugChanged(intent);
            }
            else if(action.equals(Intent.ACTION_TVDACSTATUS_CHANGED)){
				onTvDacPlugChanged(intent);
            }
            else if (action.equals(Intent.ACTION_SYNC_STATE_CHANGED)) {
                updateSyncState(intent);
            }
            else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED) ||
                    action.equals(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED)) {
                updateBluetooth(intent);
            }
            else if (action.equals(AudioManager.RINGER_MODE_CHANGED_ACTION) ||
                    action.equals(AudioManager.VIBRATE_SETTING_CHANGED_ACTION)) {
                updateVolume();
            }
            else if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                updateSimState(intent);
            }
            else if (action.equals(TtyIntent.TTY_ENABLED_CHANGE_ACTION)) {
                updateTTY(intent);
            }
        }
    };

    public PhoneStatusBarPolicy(Context context) {
        mContext = context;
        mService = (StatusBarManager)context.getSystemService(Context.STATUS_BAR_SERVICE);

        // storage
        mStorageManager = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);
        mStorageManager.registerListener(
                new com.android.systemui.usb.StorageNotification(context));

        // TTY status
        mService.setIcon("tty",  R.drawable.stat_sys_tty_mode, 0, null);
        mService.setIconVisibility("tty", false);

        // Cdma Roaming Indicator, ERI
        mService.setIcon("cdma_eri", R.drawable.stat_sys_roaming_cdma_0, 0, null);
        mService.setIconVisibility("cdma_eri", false);

        // bluetooth status
        mService.setIcon("bluetooth", R.drawable.stat_sys_data_bluetooth, 0, null);
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter != null) {
            mBluetoothEnabled = adapter.isEnabled();
        } else {
            mBluetoothEnabled = false;
        }
        mService.setIconVisibility("bluetooth", mBluetoothEnabled);

        // Alarm clock
        mService.setIcon("alarm_clock", R.drawable.stat_sys_alarm, 0, null);
        mService.setIconVisibility("alarm_clock", false);

        // Sync state
        mService.setIcon("sync_active", R.drawable.stat_sys_sync, 0, null);
        mService.setIcon("sync_failing", R.drawable.stat_sys_sync_error, 0, null);
        mService.setIconVisibility("sync_active", false);
        mService.setIconVisibility("sync_failing", false);

        // volume
        mService.setIcon("volume", R.drawable.stat_sys_ringer_silent, 0, null);
        mService.setIconVisibility("volume", false);
        updateVolume();

        IntentFilter filter = new IntentFilter();

        // Register for Intent broadcasts for...
        filter.addAction(Intent.ACTION_ALARM_CHANGED);
        filter.addAction(Intent.ACTION_SYNC_STATE_CHANGED);
        filter.addAction(AudioManager.RINGER_MODE_CHANGED_ACTION);
        filter.addAction(AudioManager.VIBRATE_SETTING_CHANGED_ACTION);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        filter.addAction(TtyIntent.TTY_ENABLED_CHANGE_ACTION);
		filter.addAction(Intent.ACTION_HDMISTATUS_CHANGED);
		filter.addAction(Intent.ACTION_TVDACSTATUS_CHANGED);
        mContext.registerReceiver(mIntentReceiver, filter, null, mHandler);
		mDisplayManager = (DisplayManager) mContext.getSystemService(Context.DISPLAY_SERVICE);
		mDispHotPolicy = new StatusBarPadHotPlug();
    }

    private final void updateAlarm(Intent intent) {
        boolean alarmSet = intent.getBooleanExtra("alarmSet", false);
        mService.setIconVisibility("alarm_clock", alarmSet);
    }

    private final void updateSyncState(Intent intent) {
        if (!SHOW_SYNC_ICON) return;
        boolean isActive = intent.getBooleanExtra("active", false);
        boolean isFailing = intent.getBooleanExtra("failing", false);
        mService.setIconVisibility("sync_active", isActive);
        // Don't display sync failing icon: BUG 1297963 Set sync error timeout to "never"
        //mService.setIconVisibility("sync_failing", isFailing && !isActive);
    }

    private final void updateSimState(Intent intent) {
        String stateExtra = intent.getStringExtra(IccCard.INTENT_KEY_ICC_STATE);
        if (IccCard.INTENT_VALUE_ICC_ABSENT.equals(stateExtra)) {
            mSimState = IccCard.State.ABSENT;
        }
        else if (IccCard.INTENT_VALUE_ICC_READY.equals(stateExtra)) {
            mSimState = IccCard.State.READY;
        }
        else if (IccCard.INTENT_VALUE_ICC_LOCKED.equals(stateExtra)) {
            final String lockedReason = intent.getStringExtra(IccCard.INTENT_KEY_LOCKED_REASON);
            if (IccCard.INTENT_VALUE_LOCKED_ON_PIN.equals(lockedReason)) {
                mSimState = IccCard.State.PIN_REQUIRED;
            }
            else if (IccCard.INTENT_VALUE_LOCKED_ON_PUK.equals(lockedReason)) {
                mSimState = IccCard.State.PUK_REQUIRED;
            }
            else {
                mSimState = IccCard.State.NETWORK_LOCKED;
            }
        } else {
            mSimState = IccCard.State.UNKNOWN;
        }
    }

    private final void updateVolume() {
        AudioManager audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        final int ringerMode = audioManager.getRingerMode();
        final boolean visible = ringerMode == AudioManager.RINGER_MODE_SILENT ||
                ringerMode == AudioManager.RINGER_MODE_VIBRATE;

        final int iconId;
        String contentDescription = null;
        if (audioManager.shouldVibrate(AudioManager.VIBRATE_TYPE_RINGER)) {
            iconId = R.drawable.stat_sys_ringer_vibrate;
            contentDescription = mContext.getString(R.string.accessibility_ringer_vibrate);
        } else {
            iconId =  R.drawable.stat_sys_ringer_silent;
            contentDescription = mContext.getString(R.string.accessibility_ringer_silent);
        }

        if (visible) {
            mService.setIcon("volume", iconId, 0, contentDescription);
        }
        if (visible != mVolumeVisible) {
            mService.setIconVisibility("volume", visible);
            mVolumeVisible = visible;
        }
    }

	private void onHdmiPlugChanged(Intent intent)
	{
		mDispHotPolicy.onHdmiPlugChanged(intent);
	}
	
	private void onTvDacPlugChanged(Intent intent)
	{
		mDispHotPolicy.onTvDacPlugChanged(intent);
	}

    private final void updateBluetooth(Intent intent) {
        int iconId = R.drawable.stat_sys_data_bluetooth;
        String contentDescription = null;
        String action = intent.getAction();
        if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            mBluetoothEnabled = state == BluetoothAdapter.STATE_ON;
        } else if (action.equals(BluetoothAdapter.ACTION_CONNECTION_STATE_CHANGED)) {
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_CONNECTION_STATE,
                BluetoothAdapter.STATE_DISCONNECTED);
            if (state == BluetoothAdapter.STATE_CONNECTED) {
                iconId = R.drawable.stat_sys_data_bluetooth_connected;
                contentDescription = mContext.getString(R.string.accessibility_bluetooth_connected);
            } else {
                contentDescription = mContext.getString(
                        R.string.accessibility_bluetooth_disconnected);
            }
        } else {
            return;
        }

        mService.setIcon("bluetooth", iconId, 0, contentDescription);
        mService.setIconVisibility("bluetooth", mBluetoothEnabled);
    }

    private final void updateTTY(Intent intent) {
        final String action = intent.getAction();
        final boolean enabled = intent.getBooleanExtra(TtyIntent.TTY_ENABLED, false);

        if (false) Slog.v(TAG, "updateTTY: enabled: " + enabled);

        if (enabled) {
            // TTY is on
            if (false) Slog.v(TAG, "updateTTY: set TTY on");
            mService.setIcon("tty", R.drawable.stat_sys_tty_mode, 0,
                    mContext.getString(R.string.accessibility_tty_enabled));
            mService.setIconVisibility("tty", true);
        } else {
            // TTY is off
            if (false) Slog.v(TAG, "updateTTY: set TTY off");
            mService.setIconVisibility("tty", false);
        }
    }

	private class StatusBarPadHotPlug implements DisplayHotPlugPolicy
    {
    	StatusBarPadHotPlug()
    	{
    		
    	}
    	
    	private void onHdmiPlugIn(Intent intent) 
		{
			int     maxscreen;
			int     maxhdmimode;
			
	        if (SHOW_HDMIPLUG_IN_CALL) 
			{
	          	Slog.d(TAG,"onHdmiPlugIn Starting!\n");
	          	mDisplayManager.setDisplayParameter(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
				maxhdmimode	= mDisplayManager.getMaxHdmiMode();
	          	mDisplayManager.setDisplayParameter(1,DisplayManager.DISPLAY_OUTPUT_TYPE_HDMI,maxhdmimode);
		        mDisplayManager.setDisplayMode(DisplayManager.DISPLAY_MODE_DUALSAME);
				maxscreen = mDisplayManager.getMaxWidthDisplay();
				//MediaPlayer.setScreen(1);
				//AudioSystem.switchAudioOutMode(AudioSystem.AUDIO_OUT_HDMI);
				//Camera.setCameraScreen(1);
		        //mDisplayManager.setDisplayOutputType(0,DisplayManager.DISPLAY_OUTPUT_TYPE_HDMI,DisplayManager.DISPLAY_TVFORMAT_1080P_60HZ);
	        }
	    }
	
		private void onTvDacYPbPrPlugIn(Intent intent)
		{
			mDisplayManager.setDisplayParameter(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
	       mDisplayManager.setDisplayParameter(1,DisplayManager.DISPLAY_OUTPUT_TYPE_TV,DisplayManager.DISPLAY_TVFORMAT_720P_60HZ);
	       mDisplayManager.setDisplayMode(DisplayManager.DISPLAY_MODE_DUALSAME);
		   //MediaPlayer.setScreen(1);
		   //Camera.setCameraScreen(1);
		}
		
		private void onTvDacCVBSPlugIn(Intent intent)
		{
		   mDisplayManager.setDisplayParameter(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
	       mDisplayManager.setDisplayParameter(1,DisplayManager.DISPLAY_OUTPUT_TYPE_TV,DisplayManager.DISPLAY_TVFORMAT_NTSC);
	       mDisplayManager.setDisplayMode(DisplayManager.DISPLAY_MODE_DUALSAME);
		   //MediaPlayer.setScreen(1);
		   //Camera.setCameraScreen(1);
		}
	
		private void onHdmiPlugOut(Intent intent)
		{
			int     maxscreen;
			
			Slog.d(TAG,"onHdmiPlugOut Starting!\n");
			mDisplayManager.setDisplayParameter(1,DisplayManager.DISPLAY_OUTPUT_TYPE_NONE,0);
	      	mDisplayManager.setDisplayParameter(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
	        mDisplayManager.setDisplayMode(DisplayManager.DISPLAY_MODE_SINGLE);
	        maxscreen = mDisplayManager.getMaxWidthDisplay();
	        //MediaPlayer.setScreen(0);
			//AudioSystem.switchAudioOutMode(AudioSystem.AUDIO_OUT_CODEC);
			//Camera.setCameraScreen(0);
	        //mDisplayManager.setDisplayOutputType(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
		}
	
		private void onTvDacPlugOut(Intent intent)
		{
			Slog.d(TAG,"onTvDacPlugOut Starting!\n");
			mDisplayManager.setDisplayParameter(1,DisplayManager.DISPLAY_OUTPUT_TYPE_NONE,0);
			mDisplayManager.setDisplayParameter(0,DisplayManager.DISPLAY_OUTPUT_TYPE_LCD,0);
	        mDisplayManager.setDisplayMode(DisplayManager.DISPLAY_MODE_SINGLE);
	        //MediaPlayer.setScreen(0);
			//Camera.setCameraScreen(0);
		}
		
		public void onHdmiPlugChanged(Intent intent)
		{
			int   hdmiplug;
			
			hdmiplug = intent.getIntExtra(DisplayManager.EXTRA_HDMISTATUS, 0);
			if(hdmiplug == 1)
			{
				onHdmiPlugIn(intent);
			}
			else
			{
				onHdmiPlugOut(intent);
			}
		}
		
		public void onTvDacPlugChanged(Intent intent)
		{
			int   tvdacplug;
			
			tvdacplug = intent.getIntExtra(DisplayManager.EXTRA_TVSTATUS, 0);
			if(tvdacplug == 1)
			{
				onTvDacYPbPrPlugIn(intent);
			}
			else if(tvdacplug == 2)
			{
				onTvDacCVBSPlugIn(intent);
			}
			else
			{
				onTvDacPlugOut(intent);
			}
		}
    }
    
    private class StatusBarTVDHotPlug implements DisplayHotPlugPolicy
    {
    	StatusBarTVDHotPlug()
    	{
    		
    	}

		public void onHdmiPlugChanged(Intent intent)
		{
		}
		
		public void onTvDacPlugChanged(Intent intent)
		{
		}
    }
}
