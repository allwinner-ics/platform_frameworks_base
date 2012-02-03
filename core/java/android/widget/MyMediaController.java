/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.widget;

import android.app.Activity;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.content.Context;
import android.widget.MediaController;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.util.Log;

public class MyMediaController extends MediaController {
    private boolean             mListenersVolumeInc;
    private boolean             mListenersVolumeDec;
    private boolean             mListenersBack;
	private View.OnClickListener mVolumeIncListener;
	private View.OnClickListener mVolumeDecListener;
	private View.OnClickListener mHomeListener;
	private View.OnClickListener mBackListener;
	ImageButton mBackButton;
	ImageButton mVolumeIncButton;
	ImageButton mVolumeDecButton;
	
	@Override
	public void onFinishInflate() {
		// TODO Auto-generated method stub
		super.onFinishInflate();
		
		addCustomView();
	}

	/* (non-Javadoc)
     * @see android.widget.MediaController#setAnchorView(android.view.View)
     */
    @Override
	public void setAnchorView(View view) {
		// TODO Auto-generated method stub
		super.setAnchorView(view);
		
		addCustomView();
	}

    private void addCustomView()
    {
    	ImageButton mPauseButton = (ImageButton) findViewById(com.android.internal.R.id.pause);
        /* set ImageButton layout params */
        LinearLayout.LayoutParams buttonLayoutParams = (LinearLayout.LayoutParams)mPauseButton.getLayoutParams();
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(buttonLayoutParams.width, 
                                                                               buttonLayoutParams.height);
        /* get parent layout */
    	LinearLayout v = (LinearLayout)mPauseButton.getParent();
    	/* set ImageButton */
    	Context mContext = v.getContext();
    	mVolumeIncButton = new ImageButton(mContext);
    	mVolumeDecButton = new ImageButton(mContext);
    	mBackButton = new ImageButton(mContext);
    	mVolumeIncButton.setImageResource(com.android.internal.R.drawable.volume_inc);
    	mVolumeDecButton.setImageResource(com.android.internal.R.drawable.volume_dec);
    	mBackButton.setImageResource(com.android.internal.R.drawable.back);
    	mVolumeIncButton.setBackgroundDrawable(null);
    	mVolumeDecButton.setBackgroundDrawable(null);
    	mBackButton.setBackgroundDrawable(null);
        if ( mVolumeIncButton != null && !mListenersVolumeInc) {
        	mVolumeIncButton.setVisibility(View.GONE);
        }
        if ( mVolumeDecButton != null && !mListenersVolumeDec) {
        	mVolumeDecButton.setVisibility(View.GONE);
        }
        if ( mBackButton != null && !mListenersBack) {
            mBackButton.setVisibility(View.GONE);
        }
        installVolumeIncListeners();
        installVolumeDecListeners();
        installBackListeners();

    	v.addView(mVolumeDecButton, 0);
    	v.addView(mVolumeIncButton, 1);
    	v.addView(mBackButton);
    }
    @Override
	public void setEnabled(boolean enabled) {
		// TODO Auto-generated method stub
		super.setEnabled(enabled);
		
        if (mVolumeIncButton != null) {
        	mVolumeIncButton.setEnabled(enabled && mVolumeIncListener != null);
        }
        if (mVolumeDecButton != null) {
        	mVolumeDecButton.setEnabled(enabled && mVolumeDecListener != null);
        }
        if (mBackButton != null) {
            mBackButton.setEnabled(enabled && mBackListener != null);
        }
	}

	private void installVolumeIncListeners() {
        if (mVolumeIncButton != null) {
        	mVolumeIncButton.setOnClickListener(mVolumeIncListener);
        	mVolumeIncButton.setEnabled(mVolumeIncListener != null);
        }
    }
	private void installVolumeDecListeners() {
        if (mVolumeDecButton != null) {
        	mVolumeDecButton.setOnClickListener(mVolumeDecListener);
        	mVolumeDecButton.setEnabled(mVolumeDecListener != null);
        }
    }
	private void installBackListeners() {
        if (mBackButton != null) {
            mBackButton.setOnClickListener(mBackListener);
            mBackButton.setEnabled(mBackListener != null);
        }
    }

    public void setVolumeIncListeners(View.OnClickListener volumeInc) {
        mVolumeIncListener = volumeInc;
        mListenersVolumeInc = true;

        installVolumeIncListeners();
        
        if (mVolumeIncButton != null) {
        	mVolumeIncButton.setVisibility(View.VISIBLE);
        }
    }
    public void setVolumeDecListeners(View.OnClickListener volumeDec) {
        mVolumeDecListener = volumeDec;
        mListenersVolumeDec = true;

        installVolumeDecListeners();
        
        if (mVolumeDecButton != null) {
        	mVolumeDecButton.setVisibility(View.VISIBLE);
        }
    }
    public void setBackListeners(View.OnClickListener back) {
        mBackListener = back;
        mListenersBack = true;

        installBackListeners();
        
        if (mBackButton != null) {
            mBackButton.setVisibility(View.VISIBLE);
        }
    }

    public MyMediaController(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

	public MyMediaController(Context context, boolean useFastForward) {
        super(context);
    }

    public MyMediaController(Context context) {
        super(context);
    }
}
