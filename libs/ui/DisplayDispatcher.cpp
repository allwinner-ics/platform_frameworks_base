//
// Copyright 2010 The Android Open Source Project
//
// The Display dispatcher.
//
#define LOG_TAG "DisplayDispatcher"

//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <ui/PowerManager.h>
#include <ui/DisplaySemaphore.h>
#include <ui/DisplayDispatcher.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define INDENT "  "
#define INDENT2 "    "

#define  MAX_FRAMEID                  2147483640

namespace android 
{
	void	DisplayDispatcherThread::enqueuebuf(int frameidx)
	{
		int   i;
		int   tmp;
		
		tmp = mFrameidx[frameidx];
		//LOGD("frameidx = %d\n",frameidx);
		//LOGD("mFrameidx0[0] = %d\n",mFrameidx[0]);
		//LOGD("mFrameidx0[1] = %d\n",mFrameidx[1]);
		//LOGD("mFrameidx0[2] = %d\n",mFrameidx[2]);
		for(i = frameidx;i > 0;i--)
		{
			mFrameidx[i] = mFrameidx[i - 1];
		}
		
		mFrameidx[0] = tmp;
		
		//LOGD("mFrameidx1[0] = %d\n",mFrameidx[0]);
		//LOGD("mFrameidx1[1] = %d\n",mFrameidx[1]);
		//LOGD("mFrameidx1[2] = %d\n",mFrameidx[2]);
	}
	
    void DisplayDispatcherThread::setSrcBuf(int srcfb_id,int srcfb_offset)
    {
        mSrcfbid        = srcfb_id;
        mSrcfboffset    = srcfb_offset;
    }

    void DisplayDispatcherThread::signalEvent()
    {
    	LOGD("signalEvent!\n");
        mSemaphore->up();
    }

    void DisplayDispatcherThread::waitForEvent()
    {
    	LOGD("waitForEvent!\n");
        mSemaphore->down();
    }
    
    void DisplayDispatcherThread::resetEvent()
    {
    	LOGD("waitForEvent!\n");
        mSemaphore->reset();
    }
    
    // --- InputDispatcherThread ---
    void DisplayDispatcherThread::LooperOnce()
    {
        int  writebufid;
        int  showbufid;
        int  ret;
        int  write_index;
        
        LOGD("before waitForEvent!\n");
        waitForEvent();
        LOGD("after waitForEvent!\n");

        mDispDevice->request_modelock(mDispDevice);

		if(mFrameidx[DISPLAYDISPATCH_MAXBUFNO - 1] == mFbOffset)
		{
        	writebufid  = mFrameidx[DISPLAYDISPATCH_MAXBUFNO - 2];

            write_index = DISPLAYDISPATCH_MAXBUFNO - 2;
    	}
    	else
    	{
    		writebufid  = mFrameidx[DISPLAYDISPATCH_MAXBUFNO - 1];

            write_index = DISPLAYDISPATCH_MAXBUFNO - 1;
    	}
    	
    	LOGD("writebufid = %d\n",writebufid);

        ret = mDispDevice->copysrcfbtodstfb(mDispDevice,mSrcfbid,1 - mSrcfboffset,1 - mSrcfbid,writebufid);
        if(ret != 0)
        {
            LOGE("copy src fb failed!\n");

            mDispDevice->release_modelock(mDispDevice);

            return ;
        }

        enqueuebuf(write_index);

        showbufid = mFrameidx[0];
        
        mFbOffset = showbufid;
        
        LOGD("showbufid = %d,mSrcfbid = %d\n",showbufid,mSrcfbid);

        mDispDevice->pandisplay(mDispDevice,1 - mSrcfbid,showbufid);

        mDispDevice->release_modelock(mDispDevice); 
    }

    DisplayDispatcherThread::DisplayDispatcherThread(display_device_t*	mDevice) :
            Thread(/*canCallJava*/ true), mDispDevice(mDevice) 
    {
    	for(int i = 0;i < DISPLAYDISPATCH_MAXBUFNO;i++)
    	{
    		mFrameidx[i] 		= i;
    	}
    	
        mSemaphore = new DisplaySemaphore(0);
    }

    DisplayDispatcherThread::~DisplayDispatcherThread() 
    {
        
    }


    bool DisplayDispatcherThread::threadLoop() 
    {
        this->LooperOnce();
        return true;
    }

    DisplayDispatcher::DisplayDispatcher()
    {
        int 				err;
        hw_module_t* 		module;
        status_t 			result;
	    
	    err = hw_get_module(DISPLAY_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	    if (err == 0) 
	    {
            LOGD("DisplayDispatcher createing1 err = %d!\n",err);
            
		    err = display_open(module, &mDevice);
		    if (err == 0) 
		    {
		    	LOGE("Open Display Device Failed!\n");
		    } 
	    } 
        else
        {
            LOGD("hw_get display module Failed!\n");
        }
	    
	    LOGD("DisplayDispatcher createing err2 = %d!\n",err);

        mThread = new DisplayDispatcherThread(mDevice);
        result = mThread->run("DisplayDispatcheR", PRIORITY_HIGHEST);
	    if (result) 
	    {
	        LOGE("Could not start DisplayDispatcheR thread due to error %d.", result);
	
	        mThread->requestExit();
	    }
    }

    DisplayDispatcher::~DisplayDispatcher()
    {
        
    }

    void DisplayDispatcher::startSwapBuffer()
    {
        int    master_bufid;
        int    master_display;
        int    mode;
        int    outputtype;
        int	   plugin;

		LOGD("startSwapBuffer!\n");
        mode            = mDevice->getdisplaymode(mDevice);
        LOGD("startSwapBuffer!mode == %d\n",mode);
        master_display  = mDevice->getmasterdisplay(mDevice);
        LOGD("startSwapBuffer!master_display = %d\n",master_display);
        outputtype		= mDevice->getdisplayparameter(mDevice,1 - master_display,DISPLAY_OUTPUT_TYPE);
        LOGD("startSwapBuffer!outputtype = %d\n",outputtype);
        if(outputtype == DISPLAY_DEVICE_HDMI)
        {
            LOGD("startSwapBuffer!outputtype == DISPLAY_DEVICE_HDMI\n");
        	plugin      = mDevice->gethdmistatus(mDevice);
             LOGD("startSwapBuffer!plugin = %d\n",plugin);
        	if(plugin  == 0)
        	{
        		mThread->resetEvent();
        		
        		return  ;
        	}

            LOGD("startSwapBuffer!mode == DISPLAY_MODE_DUALSAME1\n");
        }
        else if(outputtype == DISPLAY_DEVICE_TV)
        {
            LOGD("startSwapBuffer!outputtype == DISPLAY_DEVICE_TV\n");
        	plugin      = mDevice->gettvdacstatus(mDevice);
        	if(plugin  == 0)
        	{
        		mThread->resetEvent();
        		
        		return  ;
        	}

            LOGD("startSwapBuffer!mode == DISPLAY_MODE_DUALSAM2E\n");
        }
        
        if(mode == DISPLAY_MODE_DUALSAME)
        {
            master_bufid    = mDevice->getdisplaybufid(mDevice,master_display);

            mThread->setSrcBuf(master_display,master_bufid);
            LOGD("startSwapBuffer!mode == DISPLAY_MODE_DUALSAME3\n");
            mThread->signalEvent();
        }
    }

} // namespace android
