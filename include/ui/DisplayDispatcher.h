/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef _UI_DISPLAY_DISPATCHER_H
#define _UI_DISPLAY_DISPATCHER_H

#include <ui/Input.h>
#include <ui/InputTransport.h>
#include <utils/KeyedVector.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Looper.h>
#include <utils/BitSet.h>
#include <ui/DisplaySemaphore.h>
#include <hardware/display.h>
#include <hardware/hardware.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>

#define   DISPLAYDISPATCH_MAXBUFNO      3

#define   DISPLAY_CMD_SETDISPPARA       0
#define   DISPLAY_CMD_CHANGEDISPMODE    1
#define   DISPLAY_CMD_OPENDISP          2
#define   DISPLAY_CMD_CLOSEDISP         3
#define   DISPLAY_CMD_GETHDMISTATUS     4
#define   DISPLAY_CMD_GETTVSTATUS       5
#define   DISPLAY_CMD_GETDISPPARA       6
#define   DISPLAY_CMD_SETMASTERDISP     7
#define   DISPLAY_CMD_GETMASTERDISP     8
#define   DISPLAY_CMD_GETMAXWIDTHDISP   9
#define   DISPLAY_CMD_GETMAXHDMIMODE    10
#define   DISPLAY_CMD_GETDISPLAYMODE    11
#define   DISPLAY_CMD_GETDISPCOUNT      12
#define   DISPLAY_CMD_SETDISPMODE       13
namespace android 
{
    /* 同显时的帧管理线程 */
    class DisplayDispatcherThread : public Thread 
    {
        public:
            explicit DisplayDispatcherThread(display_device_t*	mDevice);
            ~DisplayDispatcherThread();
            void                setSrcBuf(int srcfb_id,int srcfb_offset);
            void                signalEvent();
            void                waitForEvent();
            void 				resetEvent();

        private:
            sp<DisplaySemaphore>    mSemaphore;
            int                 mSrcfbid;
            int                 mSrcfboffset;
            int                 mCurfb;             /*定义同显时需要显示副屏需要显示的fb no*/
            int                 mFbOffset;          /*定义当前在副屏上显示的fb中的buffer id*/
            int                 mFrameidx[DISPLAYDISPATCH_MAXBUFNO];  /*每个buffer管理的帧号*/
            void				enqueuebuf(int frameidx);
            virtual bool        threadLoop();
            void 				LooperOnce();
    		display_device_t*	mDispDevice;	
    };

    class DisplayDispatcher:public virtual RefBase
    {
        public:
            DisplayDispatcher();
            ~DisplayDispatcher();
            
            int     setDispProp(int cmd,int param0,int param1,int param2);
            int     getDispProp(int cmd,int param0,int param1);
            void    startSwapBuffer();
        private:
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
            sp<DisplayDispatcherThread>   mThread;
            display_device_t*	mDevice;	
            
    };

} // namespace android

#endif // _UI_INPUT_DISPATCHER_H
