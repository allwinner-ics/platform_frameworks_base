/*
 * Copyright (C) 2009 The Android Open Source Project
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

#define LOG_TAG "DisplayManagerService"

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/display.h>
#include <stdio.h>

#define LOG_NDEBUG    0

namespace android
{
    struct DisplayFieldIds 
    {
        // members
        jfieldID mDisplayOpen0;
        jfieldID mDisplayOpen1;
        jfieldID mDisplayMaster;
        jfieldID mDisplayMode;
        jfieldID mDisplayPixelFormat0;
        jfieldID mDisplayPixelFormat1;
        jfieldID mDisplayType0;
        jfieldID mDisplayType1;
        jfieldID mDisplayFormat0;
        jfieldID mDisplayFormat1;
    };
    
    static DisplayFieldIds gFieldIds;

    class NativeDisplayManager: public virtual RefBase
	{
		public:
			NativeDisplayManager();
			~NativeDisplayManager();	
			
			int  	changeDisplayMode(int displayno, int value0,int value1);
			int  	setDisplayParameter(int displayno, int value0,int value1);	
			int  	setDisplayMode(int mode);
			int		openDisplay(int displayno);
			int 	closeDisplay(int displayno);
			int 	getHdmiStatus(void);
			int 	getTvDacStatus(void);
			int 	getDisplayParameter(int displayno, int param);
			int 	setMasterDisplay(int displayno);
			int 	getMasterDisplay();
			int 	getDisplayMode();
			int 	getDisplayCount();
			int		getMaxWidthDisplay();
			int     getMaxHdmiMode();
			
			bool 	mDisplayOpen0;
	        bool 	mDisplayOpen1;
	        int 	mDisplayMaster;
	        int 	mDisplayMode;
	        int 	mDisplayPixelFormat0;
	        int 	mDisplayPixelFormat1;
	        int 	mDisplayType0;
	        int 	mDisplayType1;
	        int 	mDisplayFormat0;
	        int 	mDisplayFormat1;
		private:
			hw_module_t* 		disp_module;
    		display_device_t*	disp_device;
    		
	};
	
	NativeDisplayManager::NativeDisplayManager()
	{
		int 				err;
	    
	    err = hw_get_module(DISPLAY_HARDWARE_MODULE_ID, (hw_module_t const**)&disp_module);
	    if (err == 0) 
	    {
		    err = display_open(disp_module, &disp_device);
		    if (err == 0) 
		    {
		    	LOGE("Open Display Device Failed!\n");
		    } 
	    } 
	}
	
	NativeDisplayManager::~NativeDisplayManager()
	{
		int 				err;
	    
	    if(disp_device)
	    {
	    	display_close(disp_device);
	    }
	}

    
	
	int NativeDisplayManager::changeDisplayMode(int displayno, int value0,int value1)
	{
        int    ret;
        
		if(disp_device)
		{
			return disp_device->changemode(disp_device,displayno,value0,value1);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::setDisplayParameter(int displayno, int value0,int value1)
	{
		if(displayno == 0)
		{
			if(value0 == DISPLAY_DEVICE_LCD)
			{
				mDisplayType0 	= DISPLAY_DEVICE_LCD;
			}
			else if(value0 == DISPLAY_DEVICE_TV)
			{
				mDisplayType0 	= DISPLAY_DEVICE_TV;
				mDisplayFormat0 = value1;
			}
			else if(value0 == DISPLAY_DEVICE_HDMI)
			{
				mDisplayType0 	= DISPLAY_DEVICE_HDMI;
				mDisplayFormat0 = value1;
			}
			else if(value0 == DISPLAY_DEVICE_VGA)
			{
				mDisplayType0 	= DISPLAY_DEVICE_VGA;
				mDisplayFormat0 = value1;
			}
			else
			{
				mDisplayPixelFormat0 = value1;
			}
		}
		else
		{
			if(value0 == DISPLAY_DEVICE_LCD)
			{
				mDisplayType1 	= DISPLAY_DEVICE_LCD;
			}
			else if(value0 == DISPLAY_DEVICE_TV)
			{
				mDisplayType1 	= DISPLAY_DEVICE_TV;
				mDisplayFormat1 = value1;
			}
			else if(value0 == DISPLAY_DEVICE_HDMI)
			{
				mDisplayType1 	= DISPLAY_DEVICE_HDMI;
				mDisplayFormat1 = value1;
			}
			else if(value0 == DISPLAY_DEVICE_VGA)
			{
				mDisplayType1 	= DISPLAY_DEVICE_VGA;
				mDisplayFormat1 = value1;
			}
			else
			{
				mDisplayPixelFormat1 = value1;
			}
		}
		
		return  0;
	}
	
	
	int NativeDisplayManager::setDisplayMode(int mode)
	{
		if(disp_device)
		{
			struct display_modepara_t    disp_para;
			
			disp_para.d0type			= mDisplayType0;
    		disp_para.d1type			= mDisplayType1;
    		disp_para.d0format			= mDisplayFormat0;
    		disp_para.d1format			= mDisplayFormat1;
    		disp_para.d0pixelformat		= mDisplayPixelFormat0;
    		disp_para.d1pixelformat		= mDisplayPixelFormat1;
    		disp_para.masterdisplay		= mDisplayMaster;
    		
			return  disp_device->setdisplaymode(disp_device,mode,&disp_para);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::openDisplay(int displayno)
	{
		if(disp_device)
		{
            if(displayno == 0)
            {
                mDisplayOpen0 = true;
            }
            else
            {
                mDisplayOpen1 = true;
            }
            
			return  disp_device->opendisplay(disp_device,displayno);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::closeDisplay(int displayno)
	{
		if(disp_device)
		{
            if(displayno == 0)
            {
                mDisplayOpen0 = false;
            }
            else
            {
                mDisplayOpen1 = false;
            }
			return  disp_device->closedisplay(disp_device,displayno);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::getHdmiStatus(void)
	{
		if(disp_device)
		{
			return  disp_device->gethdmistatus(disp_device);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::getTvDacStatus(void)
	{
		if(disp_device)
		{
			return  disp_device->gettvdacstatus(disp_device);
		}
		
		return  -1;
	}
	
	int NativeDisplayManager::getDisplayParameter(int displayno, int param)
	{
		if(disp_device)
		{
			return  disp_device->getdisplayparameter(disp_device,displayno,param);
		}
		
		return  -1;
	}

    int NativeDisplayManager::setMasterDisplay(int displayno)
    {
        if(disp_device)
		{
			return  disp_device->setmasterdisplay(disp_device,displayno);
		}
		
		return  -1;
    }
    
    int NativeDisplayManager::getMasterDisplay()
    {
    	if(disp_device)
		{
			return  disp_device->getmasterdisplay(disp_device);
		}
		
		return  -1;
    }
    
    int NativeDisplayManager::getMaxWidthDisplay()
    {
    	if(disp_device)
		{
			return  disp_device->getmaxwidthdisplay(disp_device);
		}
		
		return  -1;
   	}
   	
   	int NativeDisplayManager::getMaxHdmiMode()
    {
    	if(disp_device)
		{
			return  disp_device->gethdmimaxmode(disp_device);
		}
		
		return  -1;
   	}
   	
    int NativeDisplayManager::getDisplayMode()
    {
    	if(disp_device)
		{
			return  disp_device->getdisplaymode(disp_device);
		}
		
		return  -1;
    }
    
    int NativeDisplayManager::getDisplayCount()
    {
    	if(disp_device)
		{
			return  disp_device->getdisplaycount(disp_device);
		}
		
		return  -1;
    }
    
    static sp<NativeDisplayManager> gNativeDisplayManager;
    
    static bool checkDisplayManagerUnitialized(JNIEnv* env) 
    {
	    if (gNativeDisplayManager == NULL) 
	    {
	        LOGE("Display manager not initialized.");
	        
	        jniThrowRuntimeException(env, "Display manager not initialized.");
	        
	        return true;
	    }
	    return false;
	}

    
	
	static void init_native(JNIEnv *env, jobject clazz)
	{
	    if(gNativeDisplayManager == NULL)
	    {
	    	gNativeDisplayManager = new NativeDisplayManager();
	    }
	    else 
	    {
	        LOGE("Native Display manager already initialized.");
	        
	        jniThrowRuntimeException(env, "Display manager already initialized.");
	    }
	}

    
	
	static void finalize_native(JNIEnv *env, jobject clazz)
	{
	    
	}

    static void updateStatus(JNIEnv *env, jobject clazz)
    {
        env->SetIntField(clazz, gFieldIds.mDisplayFormat0, gNativeDisplayManager->mDisplayFormat0);
        env->SetIntField(clazz, gFieldIds.mDisplayFormat1, gNativeDisplayManager->mDisplayFormat1);
        env->SetIntField(clazz, gFieldIds.mDisplayType0, gNativeDisplayManager->mDisplayType0);
        env->SetIntField(clazz, gFieldIds.mDisplayType1, gNativeDisplayManager->mDisplayType1);
        env->SetIntField(clazz, gFieldIds.mDisplayPixelFormat0, gNativeDisplayManager->mDisplayPixelFormat0);
        env->SetIntField(clazz, gFieldIds.mDisplayPixelFormat1, gNativeDisplayManager->mDisplayPixelFormat1);
        env->SetIntField(clazz, gFieldIds.mDisplayMaster, gNativeDisplayManager->mDisplayMaster);
        env->SetIntField(clazz, gFieldIds.mDisplayMode, gNativeDisplayManager->mDisplayMode);
        env->SetBooleanField(clazz, gFieldIds.mDisplayOpen0, gNativeDisplayManager->mDisplayOpen0);
        env->SetBooleanField(clazz, gFieldIds.mDisplayOpen1, gNativeDisplayManager->mDisplayOpen1);
    }

	static jint changeMode_native(JNIEnv *env, jobject clazz,int displayno, int value0,int value1)
	{
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("changeMode_native.");
	    
	    ret = gNativeDisplayManager->changeDisplayMode(displayno,value0,value1);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;
	}

    static jint setDisplayParameter_native(JNIEnv *env, jobject clazz,int displayno, int value0,int value1)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("setDisplayParameter_native.");
	    
	    ret = gNativeDisplayManager->setDisplayParameter(displayno,value0,value1);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;	
    }

    static jint setDisplayMode_native(JNIEnv *env, jobject clazz,int mode)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("setDisplayMode_native.");
	    
	    ret = gNativeDisplayManager->setDisplayMode(mode);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;
    }

    static jint openDisplay_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("openDisplay_native.");
	    
	    ret = gNativeDisplayManager->openDisplay(displayno);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;	
    }


    static jint closeDisplay_native(JNIEnv *env, jobject clazz, int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("closeDisplay_native.");
	    
	    ret = gNativeDisplayManager->closeDisplay(displayno);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;		
    }


    static jint getHdmiStatus_native(JNIEnv *env, jobject clazz)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
        
       // LOGE("getHdmiStatus_native.");
        
	    return  (jint)gNativeDisplayManager->getHdmiStatus();		
    }
    
    static jint getMaxHdmiMode_native(JNIEnv *env, jobject clazz)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
        
       // LOGE("getHdmiStatus_native.");
        
	    return  (jint)gNativeDisplayManager->getMaxHdmiMode();		
    }


    static jint getTvDacStatus_native(JNIEnv *env, jobject clazz)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getTvDacStatus_native.");
        
	    return  (jint)gNativeDisplayManager->getTvDacStatus();	
    }

    static jint getDisplayParameter_native(JNIEnv *env, jobject clazz,int displayno, int param)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayParameter_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,param);
    }

    static jint getDisplayMode_native(JNIEnv *env, jobject clazz)
    {
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayMode_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayMode();
    }

    static jint getDisplayCount_native(JNIEnv *env, jobject clazz)
    {
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayCount_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayCount();
    }

    static jint setDisplayMaster_native(JNIEnv *env, jobject clazz,int displayno)
    {
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    LOGE("setDisplayMaster_native.");
	    
	    ret = gNativeDisplayManager->setMasterDisplay(displayno);
        if(ret == 0)
        {
            updateStatus(env, clazz);
        }

	    return  (jint)ret;
    }

    static jint getDisplayMaster_native(JNIEnv *env, jobject clazz)
    {
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayMaster_native.");
	    
        
	    return  (jint)gNativeDisplayManager->getMasterDisplay();
    }
    
    static jint getMaxWidthDisplay_native(JNIEnv *env, jobject clazz)
    {
        int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayMaster_native.");
	    
        
	    return  (jint)gNativeDisplayManager->getMaxWidthDisplay();
    }

    
    static jint getDisplayOutputType_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    } 
	    
	    //LOGE("getDisplayOutputType_native."); 
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_TYPE);
    }
    
    static jint getDisplayOutputFormat_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputFormat_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_FORMAT);
    }
    
    static jint getDisplayOutputWidth_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputWidth_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_WIDTH);
    }
    
    static jint getDisplayOutputHeight_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputHeight_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_HEIGHT);
    }
    
    static jint getDisplayOutputPixelFormat_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputPixelFormat_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_PIXELFORMAT);
    }
    
    static jint getDisplayOutputOpen_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputOpen_native.");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_ISOPEN);
    }
    
    static jint getDisplayOutputHotPlug_native(JNIEnv *env, jobject clazz,int displayno)
    {
    	int   ret;
        
        //LOGE("Native Display manager already initialized.");
        
		if (checkDisplayManagerUnitialized(env)) 
		{
	        return -1;
	    }  
	    
	    //LOGE("getDisplayOutputHotPlug_native!\n");
        
	    return  (jint)gNativeDisplayManager->getDisplayParameter(displayno,DISPLAY_OUTPUT_HOTPLUG);
    }

static JNINativeMethod method_table[] = {
    { "nativeInit", "()V", (void*)init_native },
    { "nativeSetDisplayOutputType", "(III)I", (void*)changeMode_native },
    { "nativeSetDisplayParameter", "(III)I", (void*)setDisplayParameter_native },
    { "nativeSetDisplayMode", "(I)I", (void*)setDisplayMode_native },
    { "nativeOpenDisplay", "(I)I", (void*)openDisplay_native },
    { "nativeCloseDisplay", "(I)I", (void*)closeDisplay_native },
    { "nativeGetHdmiHotPlug", "()I", (void*)getHdmiStatus_native },
    { "nativeGetTvDacHotPlug", "()I", (void*)getTvDacStatus_native },
    { "nativeGetDisplayMode", "()I", (void*)getDisplayMode_native },
    { "nativeGetDisplayCount", "()I", (void*)getDisplayCount_native },
    { "nativeSetDisplayMaster", "(I)I", (void*)setDisplayMaster_native },
    { "nativeGetDisplayMaster", "()I", (void*)getDisplayMaster_native },
    { "nativeGetMaxWidthDisplay", "()I", (void*)getMaxWidthDisplay_native },
    { "nativeGetMaxHdmiMode", "()I", (void*)getMaxHdmiMode_native },
    { "nativeGetDisplayOutputType", "(I)I", (void*)getDisplayOutputType_native },
    { "nativeGetDisplayOutputFormat", "(I)I", (void*)getDisplayOutputFormat_native},
    { "nativeGetDisplayWidth", "(I)I", (void*)getDisplayOutputWidth_native },
    { "nativeGetDisplayHeight", "(I)I", (void*)getDisplayOutputHeight_native },
    { "nativeGetDisplayPixelFormat", "(I)I", (void*)getDisplayOutputPixelFormat_native },
    { "nativeGetDisplayOpen", "(I)I", (void*)getDisplayOutputOpen_native },
    { "nativeGetDisplayHotPlug", "(I)I", (void*)getDisplayOutputHotPlug_native },
};

int register_android_server_DisplayManagerService(JNIEnv *env)
{
	jclass clazz = env->FindClass("com/android/server/DisplayManagerService");

    if (clazz == NULL) 
    {
        LOGE("Can't find com/android/server/DisplayManagerService");
        
        return -1;
    }
    
    gFieldIds.mDisplayMode 			= env->GetFieldID(clazz, "mDisplayMode", "I");
    gFieldIds.mDisplayOpen0 		= env->GetFieldID(clazz, "mDisplayOpen0", "Z");
    gFieldIds.mDisplayOpen1 		= env->GetFieldID(clazz, "mDisplayOpen1", "Z");
    gFieldIds.mDisplayMaster 		= env->GetFieldID(clazz, "mDisplayMaster", "I");
    gFieldIds.mDisplayPixelFormat0 	= env->GetFieldID(clazz, "mDisplayPixelFormat0", "I");
    gFieldIds.mDisplayPixelFormat1 	= env->GetFieldID(clazz, "mDisplayPixelFormat1", "I");
    gFieldIds.mDisplayType0 		= env->GetFieldID(clazz, "mDisplayType0", "I");
    gFieldIds.mDisplayType1 		= env->GetFieldID(clazz, "mDisplayType1", "I");
    gFieldIds.mDisplayFormat0 		= env->GetFieldID(clazz, "mDisplayFormat0", "I");
    gFieldIds.mDisplayFormat1 		= env->GetFieldID(clazz, "mDisplayFormat1", "I");
    
    return jniRegisterNativeMethods(env, "com/android/server/DisplayManagerService",
            method_table, NELEM(method_table));
}

};
