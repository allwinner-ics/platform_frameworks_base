/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.server;

import com.android.internal.app.IMediaContainerService;
import com.android.internal.app.ResolverActivity;
import com.android.internal.content.NativeLibraryHelper;
import com.android.internal.content.PackageHelper;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.JournaledFile;
import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.admin.IDevicePolicyManager;
import android.app.backup.IBackupManager;
import android.content.Context;
import android.content.ComponentName;
import android.content.IIntentReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.ServiceConnection;
import android.content.IntentSender.SendIntentException;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.ComponentInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageDeleteObserver;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.IPackageManager;
import android.content.pm.IPackageMoveObserver;
import android.content.pm.IPackageStatsObserver;
import android.content.pm.InstrumentationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInfoLite;
import android.content.pm.PackageManager;
import android.content.pm.PackageStats;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import android.content.pm.PackageParser;
import android.content.pm.PermissionInfo;
import android.content.pm.PermissionGroupInfo;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.Environment;
import android.os.FileObserver;
import android.os.FileUtils;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.security.SystemKeyStore;
import android.util.*;
import android.view.Display;
import android.view.WindowManager;
import android.view.IWindowManager;
import android.view.IDisplayManager;
import android.view.DisplayManager;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * Keep track of all those .apks everywhere.
 * 
 * This is very central to the platform's security; please run the unit
 * tests whenever making modifications here:
 * 
mmm frameworks/base/tests/AndroidTests
adb install -r -f out/target/product/passion/data/app/AndroidTests.apk
adb shell am instrument -w -e class com.android.unit_tests.PackageManagerTests com.android.unit_tests/android.test.InstrumentationTestRunner
 *
 */
public class DisplayManagerService extends IDisplayManager.Stub 
{
    private static final String TAG = "DisplayManagerService";

    private static final boolean LOCAL_LOGV = false;
	
    private final Context 	mContext;
	private final 			PowerManagerService mPM;
	private IWindowManager 	mWindowManager;
    private int 			mHdmiPlugin;
    private int 			mTvDacPlugin;
	private boolean			mDisplayOpen0;
	private boolean 		mDisplayOpen1;
	private int				mDisplayMaster;
	private int     		mDisplayMode;
	private int				mDisplayPixelFormat0;
	private int 			mDisplayPixelFormat1;
    private int 			mDisplayType0;
    private int 			mDisplayType1;
	private int 			mDisplayFormat0;
	private int 			mDisplayFormat1;
	private static 			DisplayThread sThread;
    private static 			boolean sThreadStarted = false;
    
    private native void nativeInit();
    private native int 	nativeGetDisplayCount();
    private native int 	nativeGetDisplayOutputType(int mDisplay);
    private native int  nativeGetDisplayOutputFormat(int mDisplay);
    private native int  nativeGetDisplayWidth(int mDisplay);
    private native int  nativeGetDisplayHeight(int mDisplay);
    private native int  nativeGetDisplayPixelFormat(int mDisplay);
    private native int  nativeSetDisplayMode(int mode);
    private native int  nativeSetDisplayOutputType(int mDisplay,int type,int format);
    private native int  nativeOpenDisplay(int mDisplay);
    private native int  nativeCloseDisplay(int mDisplay);
    private native int  nativeSetDisplayMaster(int mDisplay);
    private native int  nativeGetDisplayOpen(int mDisplay);
    private native int  nativeGetDisplayHotPlug(int mDisplay);
	private native int  nativeGetHdmiHotPlug();
	private native int  nativeGetTvDacHotPlug();
	private native int  nativeGetDisplayMode();
	private native int  nativeGetDisplayMaster();
	private native int  nativeGetMaxWidthDisplay();
	private native int  nativeGetMaxHdmiMode();
	private native int  nativeSetDisplayParameter(int mDisplay,int para0,int para1);

	private final void sendHdmiIntent() 
	{
        //  Pack up the values and broadcast them to everyone
        Intent intent = new Intent(Intent.ACTION_HDMISTATUS_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
		
        intent.putExtra(DisplayManager.EXTRA_HDMISTATUS, mHdmiPlugin);

        if (true) 
		{
            Slog.d(TAG, "mHdmiPlugin:" + mHdmiPlugin);
        }

        ActivityManagerNative.broadcastStickyIntent(intent, null);
    }

	private final void sendTvDacIntent() 
	{
        //  Pack up the values and broadcast them to everyone
        Intent intent = new Intent(Intent.ACTION_TVDACSTATUS_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
		
        intent.putExtra(DisplayManager.EXTRA_TVSTATUS, mTvDacPlugin);

        if (true) 
		{
            Slog.d(TAG, "mTvDacPlugin:" + mTvDacPlugin);
        }

        ActivityManagerNative.broadcastStickyIntent(intent, null);
    }

	private final void update_hotplug()
	{
		mHdmiPlugin 	= nativeGetHdmiHotPlug();
		if(mHdmiPlugin != 0)
		{
			sendHdmiIntent();
		}	
		mTvDacPlugin 	= nativeGetTvDacHotPlug();
		if(mTvDacPlugin != 0)
		{
			sendTvDacIntent();
		}
	}
	
	public class DisplayThread extends Thread 
	{
        private final DisplayManagerService mService;
        private final Context mContext;
        private final PowerManagerService mPM;
        boolean mRunning = false;
		private	int hdmihotplug;
		private int tvdachotplug;

        public DisplayThread(Context context,DisplayManagerService service,
                PowerManagerService pm) 
        {
            super("DisplayManagerPolicy");
            mService = service;
            mContext = context;
            mPM = pm;
        }

		public boolean hdmiStatusChange()
		{
			if(hdmihotplug == mService.getDisplayHdmiHotPlug())
			{
				return  false;
			}

			return  true;
		}

		public boolean tvStatusChange()
		{
			if(tvdachotplug == mService.getDisplayTvDacHotPlug())
			{
				return  false;
			}

			return  true;
		}

        public void run() 
		{
            //Looper.prepare();
            
            android.os.Process.setThreadPriority(
                    android.os.Process.THREAD_PRIORITY_FOREGROUND);
            android.os.Process.setCanSelfBackground(false);

            synchronized (this) 
			{
                mRunning = true;
                notifyAll();
            }

			while (true) 
			{
				int  hotplug;
				
				//Log.d(TAG,"HDMI System.currentTimeMillis() = " + System.currentTimeMillis());
                hotplug = nativeGetHdmiHotPlug();
				if(hotplug >= 0)
				{
					hdmihotplug = hotplug;
				}
				
                //Log.d(TAG,"HDMI2 System.currentTimeMillis() = " + System.currentTimeMillis());
				if(hdmiStatusChange())
				{
					Log.d(TAG,"changed hdmihotplug = " + hdmihotplug);
					mService.setDisplayHdmiHotPlug(hdmihotplug);

					sendHdmiIntent();
				}
				else
				{
					//Log.d(TAG,"nochanged hdmihotplug = " + hdmihotplug);
				}

				//Log.d(TAG,"TV1 System.currentTimeMillis() = " + System.currentTimeMillis());
				hotplug = nativeGetTvDacHotPlug();
				if(hotplug >= 0)
				{
					tvdachotplug = hotplug;
				}
				//Log.d(TAG,"TV2 System.currentTimeMillis() = " + System.currentTimeMillis());
				if(tvStatusChange())
				{
					Log.d(TAG,"changed tvdachotplug = " + tvdachotplug);
					mService.setDisplayTvDacHotPlug(tvdachotplug);
//
					sendTvDacIntent();
				}
				else
				{
					//Log.d(TAG,"nochanged tvdachotplug = " + tvdachotplug);
				}

				try 
				{
		            Thread.sleep(500);
		        } 
		        catch (Exception e) 
		        {
		            Log.d(TAG,"thread sleep failed!");
		        } 
				
            }

            //Looper.loop();
        }
    }

	
	
    public DisplayManagerService(Context context,PowerManagerService pm) 
    {
        mContext = context;
		mPM		 = pm;

		Log.d(TAG,"DisplayManagerService Starting.......!");
		
		nativeInit();
		
        // set initial hotplug status
        update_hotplug();

		if (sThreadStarted == false) 
		{
            sThread = new DisplayThread(mContext,this,mPM);
            sThread.start();
            sThreadStarted = true;
        }

		mWindowManager	= IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
		Log.d(TAG,"getWindowManager Starting.......!");
    }
    
    public void systemReady()
    {
    	
    }
    
	public int getDisplayCount()
	{
		return nativeGetDisplayCount();
	}
	
	public int getDisplayOutputType(int mDisplay)
	{
		return nativeGetDisplayOutputType(mDisplay);
	}
	
	public int getDisplayOutputFormat(int mDisplay)
	{
		return nativeGetDisplayOutputFormat(mDisplay);
	}
	
	public int getDisplayWidth(int mDisplay)
	{
		return nativeGetDisplayWidth(mDisplay);
	}
	
	public int getDisplayHeight(int mDisplay)
	{
		return nativeGetDisplayHeight(mDisplay);
	}
	
	public int getDisplayPixelFormat(int mDisplay)
	{
		return nativeGetDisplayPixelFormat(mDisplay);
	}
	
	public int setDisplayParameter(int mDisplay,int param0,int param1)
	{
		return nativeSetDisplayParameter(mDisplay,param0,param1);
	}
	
	public int setDisplayMode(int mode)
	{
		int    ret;
		int    wmret;
		
		Log.d(TAG,"setDisplayMode!");
		ret = nativeSetDisplayMode(mode);
		if(ret == 0)
		{
			try 
			{
	            //mWindowManager.displayModeChanged(true);
	            Log.d(TAG,"thread setDisplayMode failed!");
	        } 
	        catch (Exception e) 
	        {
	        	ret  = -1;
	            Log.d(TAG,"thread displayModeChanged failed!");
	        } 
		}
		
		Log.d(TAG,"thread setDisplayMode2 failed!");

		return ret;
	}
	
	public int setDisplayOutputType(int mDisplay,int type,int format)
	{
		int    ret;
		int    wmret;
		
		ret = nativeSetDisplayOutputType(mDisplay,type,format);
		Log.d(TAG,"thread displayModeChanged ret = " + ret);
		if(ret == 0)
		{
			try 
			{
				Log.d(TAG,"thread displayModeChanged Starting!");
	            //mWindowManager.displayModeChanged(true);
	        } 
	        catch (Exception e) 
	        {
	        	ret  = -1;
	            Log.d(TAG,"thread displayModeChanged failed!");
	        } 
		}

		return ret;
	}

	public int getDisplayHdmiHotPlug()
	{
		return mHdmiPlugin;
	}

	public int getDisplayTvDacPlugStatus(int mDisplay)
	{
		return mTvDacPlugin;
	}

	public void setDisplayHdmiHotPlug(int hotplug)
	{
		mHdmiPlugin = hotplug;
	}

	public int getDisplayTvDacHotPlug()
	{
		return mTvDacPlugin;
	}

	public void setDisplayTvDacHotPlug(int hotplug)
	{
		mTvDacPlugin = hotplug;
	}
	
	public int getDisplayHotPlugStatus(int mDisplay)
	{
		return nativeGetDisplayHotPlug(mDisplay);
	}
	
	public int openDisplay(int mDisplay)
	{
		return nativeOpenDisplay(mDisplay);
	}
	
	public int closeDisplay(int mDisplay)
	{
		return nativeCloseDisplay(mDisplay);
	}
	
	public boolean getDisplayOpenStatus(int mDisplay)
	{
		int  ret;
		
		ret = nativeGetDisplayOpen(mDisplay);
		if(ret == 0)
		{
			return false;
		}
		
		return true;
	}

	public int getDisplayMode()
	{
		return nativeGetDisplayMode();
	}
	
	public int setDisplayMaster(int mDisplay)
	{
		int    ret;
		int    wmret;
		
		ret = setDisplayMaster(mDisplay);
		if(ret == 0)
		{
			try 
			{
	            //mWindowManager.displayModeChanged(true);
	        } 
	        catch (Exception e) 
	        {
	        	ret  = -1;
	            Log.d(TAG,"thread displayModeChanged failed!");
	        } 
		}

		return ret;
	}

	public int getDisplayMaster()
	{
		return nativeGetDisplayMaster();
	}
	
	public int getMaxWidthDisplay()
	{
		return nativeGetMaxWidthDisplay();
	}
	
	public int getMaxHdmiMode()
	{
		return nativeGetMaxHdmiMode();
	}
}

