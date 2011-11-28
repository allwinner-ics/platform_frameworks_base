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

#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXPlayer"
#include <utils/Log.h>
 
#include <dlfcn.h>

#include "CedarXPlayer.h"
#include <libcedarv.h>

#include <binder/IPCThreadState.h>
#include <media/stagefright/CedarXAudioPlayer.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/VideoRenderer.h>

#include <surfaceflinger/ISurface.h>

#include <media/stagefright/foundation/ALooper.h>

#include <OMX_IVCommon.h>
#include <hardware/overlay.h>

namespace android {

//static int64_t kLowWaterMarkUs = 2000000ll; // 2secs
//static int64_t kHighWaterMarkUs = 10000000ll; // 10secs

extern "C" int CedarXPlayerCallbackWrapper(void *cookie, int event, void *info);

struct CedarXLocalRenderer: public CedarXRenderer {
	CedarXLocalRenderer(bool previewOnly, const char *componentName,
			OMX_COLOR_FORMATTYPE colorFormat, const sp<ISurface> &surface,
			size_t displayWidth, size_t displayHeight, size_t decodedWidth,
			size_t decodedHeight, int32_t rotationDegrees, int32_t screenId) :
		mTarget(NULL), mLibHandle(NULL) {
		init(previewOnly, componentName, colorFormat, surface, displayWidth,
				displayHeight, decodedWidth, decodedHeight, rotationDegrees, screenId);
	}

	virtual void render(const void *data, size_t size) {
		mTarget->render(data, size, NULL);
	}
#ifndef __CHIP_VERSION_F20
	virtual int control(int cmd, int para){
		return mTarget->control(cmd, para);
	}
#else
	virtual int getframeid(){
		return mTarget->getframeid();
	}
#endif

protected:
	virtual ~CedarXLocalRenderer() {
		delete mTarget;
		mTarget = NULL;

		if (mLibHandle) {
			dlclose(mLibHandle);
			mLibHandle = NULL;
		}
	}

private:
	VideoRenderer *mTarget;
	void *mLibHandle;

	void init(bool previewOnly, const char *componentName,
			OMX_COLOR_FORMATTYPE colorFormat, const sp<ISurface> &surface,
			size_t displayWidth, size_t displayHeight, size_t decodedWidth,
			size_t decodedHeight, int32_t rotationDegrees, int32_t screenId);

	CedarXLocalRenderer(const CedarXLocalRenderer &);
	CedarXLocalRenderer &operator=(const CedarXLocalRenderer &);
	;
};

void CedarXLocalRenderer::init(bool previewOnly, const char *componentName,
		OMX_COLOR_FORMATTYPE colorFormat, const sp<ISurface> &surface,
		size_t displayWidth, size_t displayHeight, size_t decodedWidth,
		size_t decodedHeight, int32_t rotationDegrees, int32_t screenId) {
	if (!previewOnly) {
		// We will stick to the vanilla software-color-converting renderer
		// for "previewOnly" mode, to avoid unneccessarily switching overlays
		// more often than necessary.

		mLibHandle = dlopen("libstagefrighthw.so", RTLD_NOW);

		if (mLibHandle) {
			typedef VideoRenderer *(*CreateRendererWithRotationFunc)(const sp<
					ISurface> &surface, const char *componentName,
					OMX_COLOR_FORMATTYPE colorFormat, size_t displayWidth,
					size_t displayHeight, size_t decodedWidth,
					size_t decodedHeight, int32_t rotationDegrees);

			typedef VideoRenderer *(*CreateRendererFunc)(
					const sp<ISurface> &surface, const char *componentName,
					OMX_COLOR_FORMATTYPE colorFormat, size_t displayWidth,
					size_t displayHeight, size_t decodedWidth,
					size_t decodedHeight,int32_t screenId);

			CreateRendererWithRotationFunc funcWithRotation =
					(CreateRendererWithRotationFunc) dlsym(mLibHandle,
							"_Z26createRendererWithRotationRKN7android2spINS_8"
								"ISurfaceEEEPKc20OMX_COLOR_FORMATTYPEjjjji");

			if (funcWithRotation) {
				mTarget = (*funcWithRotation)(surface, componentName,
						colorFormat, displayWidth, displayHeight, decodedWidth,
						decodedHeight, rotationDegrees);
			} else {
				if (rotationDegrees != 0) {
					LOGW("renderer does not support rotation.");
				}
#ifndef __CHIP_VERSION_F20
				CreateRendererFunc func = (CreateRendererFunc) dlsym(
						mLibHandle,
						"_Z14createRendererRKN7android2spINS_8ISurfaceEEEPKc20"
							"OMX_COLOR_FORMATTYPEjjjji");
#else
				CreateRendererFunc func = (CreateRendererFunc) dlsym(
						mLibHandle,
						"_Z14createRendererRKN7android2spINS_8ISurfaceEEEPKc20"
							"OMX_COLOR_FORMATTYPEjjjj");
#endif
				if (func) {
					mTarget = (*func)(surface, componentName, colorFormat,
							displayWidth, displayHeight, decodedWidth,
							decodedHeight, screenId);
				}
			}
		}
	}

	if (mTarget == NULL) {
		LOGE("create render error!");
	}
}

CedarXPlayer::CedarXPlayer() :
	mQueueStarted(false), mVideoRendererIsPreview(false),
	mAudioPlayer(NULL), mFlags(0), mExtractorFlags(0), mCanSeek(0){

	LOGV("Construction");
	reset_l();
	CDXPlayer_Create((void**)&mPlayer);

	mPlayer->control(mPlayer, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXPlayerCallbackWrapper, (unsigned int)this);
	isCedarXInitialized = true;
}

CedarXPlayer::~CedarXPlayer() {

	if(isCedarXInitialized){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
		CDXPlayer_Destroy(mPlayer);
		mPlayer = NULL;
		isCedarXInitialized = false;
	}

	if (mAudioPlayer) {
		LOGV("delete mAudioPlayer");
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}

	LOGV("Deconstruction %x",mFlags);
}

void CedarXPlayer::setListener(const wp<MediaPlayerBase> &listener) {
	Mutex::Autolock autoLock(mLock);
	mListener = listener;
}

status_t CedarXPlayer::setDataSource(const char *uri, const KeyedVector<
		String8, String8> *headers) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::setDataSource (%s)", uri);
	mUri = uri;
	mIsUri = true;
	if (headers) {
	    mUriHeaders = *headers;
	}

	mPlayer->control(mPlayer, CDX_SET_DATASOURCE_URL, (unsigned int)uri, 0);

	return OK;
}

status_t CedarXPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
	//Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::setDataSource fd");
	CedarXExternFdDesc ext_fd_desc;
	ext_fd_desc.fd = fd;
	ext_fd_desc.offset = offset;
	ext_fd_desc.length = length;
	mIsUri = false;
	mPlayer->control(mPlayer, CDX_SET_DATASOURCE_FD, (unsigned int)&ext_fd_desc, 0);
	return OK;
}

void CedarXPlayer::reset() {
	//Mutex::Autolock autoLock(mLock);
	LOGV("RESET ???????????????????????????????, context: %p",this);

	if(mPlayer != NULL){
		mPlayer->control(mPlayer, CDX_CMD_RESET, 0, 0);
		if(isCedarXInitialized){
			mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
			CDXPlayer_Destroy(mPlayer);
			mPlayer = NULL;
			isCedarXInitialized = false;
		}
	}

	reset_l();
}

void CedarXPlayer::reset_l() {
	LOGV("reset_l");

	mPlayerState = PLAYER_STATE_UNKOWN;
	pause_l(true);
	mVideoRenderer.clear();
	mVideoRenderer = NULL;
	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
	LOGV("RESET End");

	mDurationUs = 0;
	mFlags = 0;
	mFlags |= RESTORE_CONTROL_PARA;
	mVideoWidth = mVideoHeight = -1;
	mVideoTimeUs = 0;

	mTagPlay = true;
	mSeeking = false;
	mSeekNotificationSent = false;
	mSeekTimeUs = 0;
	mLastValidPosition = 0;

	mAudioTrackIndex = 0;
	memset(&mSubtitleParameter, 0, sizeof(struct SubtitleParameter));

	mBitrate = -1;
}

void CedarXPlayer::notifyListener_l(int msg, int ext1, int ext2) {
	if (mListener != NULL) {
		sp<MediaPlayerBase> listener = mListener.promote();

		if (listener != NULL) {
			listener->sendEvent(msg, ext1, ext2);
		}
	}
}

status_t CedarXPlayer::play() {
	Mutex::Autolock autoLock(mLock);
	LOGV("CedarXPlayer::play()");

	mFlags &= ~CACHE_UNDERRUN;

	status_t ret = play_l(CDX_CMD_START_ASYNC);

	LOGV("CedarXPlayer::play() end");
	return ret;
}

status_t CedarXPlayer::play_l(int command) {
	LOGV("CedarXPlayer::play_l()");

	if (mFlags & PLAYING) {
		return OK;
	}

    if (!(mFlags & PREPARED)) {
        status_t err = prepare_l();

        if (err != OK) {
            return err;
        }
    }

	mFlags |= PLAYING;

	if(mAudioPlayer){
		mAudioPlayer->resume();
	}

	if(mFlags & RESTORE_CONTROL_PARA){
		if(mMediaInfo.mSubtitleStreamCount > 0) {
			LOGV("Restore control parameter!");
			if(mSubtitleParameter.mSubtitleDelay != 0){
				setSubDelay(mSubtitleParameter.mSubtitleDelay);
			}

			if(mSubtitleParameter.mSubtitleColor != 0){
				setSubColor(mSubtitleParameter.mSubtitleColor);
			}

			if(mSubtitleParameter.mSubtitleFontSize != 0){
				setSubFontSize(mSubtitleParameter.mSubtitleFontSize);
			}

			if(mSubtitleParameter.mSubtitlePosition != 0){
				setSubPosition(mSubtitleParameter.mSubtitlePosition);
			}

			setSubGate(mSubtitleParameter.mSubtitleGate);
		}
		mFlags &= ~RESTORE_CONTROL_PARA;
	}

	if(mSeeking && mTagPlay && mSeekTimeUs > 0){
		mPlayer->control(mPlayer, CDX_CMD_TAG_START_ASYNC, (unsigned int)&mSeekTimeUs, 0);
		LOGD("--tag play %lldus",mSeekTimeUs);
	}
	else if(mPlayerState == PLAYER_STATE_SUSPEND || mPlayerState == PLAYER_STATE_RESUME){
		mPlayer->control(mPlayer, CDX_CMD_TAG_START_ASYNC, (unsigned int)&mSuspensionState.mPositionUs, 0);
		LOGD("--tag play %lldus",mSuspensionState.mPositionUs);
	}
	else {
		mPlayer->control(mPlayer, command, (unsigned int)&mSuspensionState.mPositionUs, 0);
	}

	mTagPlay = false;
	mPlayerState = PLAYER_STATE_PLAYING;
	mFlags &= ~PAUSING;

	return OK;
}

status_t CedarXPlayer::stop() {
	LOGV("CedarXPlayer::stop");

	if(mPlayer != NULL){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
	}
	stop_l();

	return OK;
}

status_t CedarXPlayer::stop_l() {
	LOGV("stop() status:%x", mFlags & PLAYING);

	notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
	LOGD("MEDIA_PLAYBACK_COMPLETE");
	notifyListener_l(MEDIA_PLAYBACK_COMPLETE);

	pause_l(true);

	mVideoRenderer.clear();
	mVideoRenderer = NULL;
	mFlags &= ~SUSPENDING;
	LOGV("stop finish 1...");

	return OK;
}

status_t CedarXPlayer::pause() {
	//Mutex::Autolock autoLock(mLock);
	LOGV("pause cmd");
#if 0
	mFlags &= ~CACHE_UNDERRUN;
	mPlayer->control(mPlayer, CDX_CMD_PAUSE, 0, 0);

	return pause_l(false);
#else
	if (!(mFlags & PLAYING)) {
		return OK;
	}

	pause_l(false);
	mPlayer->control(mPlayer, CDX_CMD_PAUSE_ASYNC, 0, 0);

	return OK;
#endif
}

status_t CedarXPlayer::pause_l(bool at_eos) {

	mPlayerState = PLAYER_STATE_PAUSE;

	if (!(mFlags & PLAYING)) {
		return OK;
	}

	if (mAudioPlayer != NULL) {
		if (at_eos) {
			// If we played the audio stream to completion we
			// want to make sure that all samples remaining in the audio
			// track's queue are played out.
			mAudioPlayer->pause(true /* playPendingSamples */);
		} else {
			mAudioPlayer->pause();
		}
	}

	mFlags &= ~PLAYING;
	mFlags |= PAUSING;

	return OK;
}

bool CedarXPlayer::isPlaying() const {
	//LOGV("isPlaying cmd mFlags=0x%x",mFlags);
	return (mFlags & PLAYING) || (mFlags & CACHE_UNDERRUN);
}

void CedarXPlayer::setISurface(const sp<ISurface> &isurface) {
	//Mutex::Autolock autoLock(mLock);
	mISurface = isurface;
}

void CedarXPlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink) {
	//Mutex::Autolock autoLock(mLock);
	mAudioSink = audioSink;
}

status_t CedarXPlayer::setLooping(bool shouldLoop) {
	//Mutex::Autolock autoLock(mLock);

	mFlags = mFlags & ~LOOPING;

	if (shouldLoop) {
		mFlags |= LOOPING;
	}

	return OK;
}

status_t CedarXPlayer::getDuration(int64_t *durationUs) {

	mPlayer->control(mPlayer, CDX_CMD_GET_DURATION, (unsigned int)durationUs, 0);
	*durationUs *= 1000;
	mDurationUs = *durationUs;

	return OK;
}

status_t CedarXPlayer::getPosition(int64_t *positionUs) {

	if(mFlags & AT_EOS){
		*positionUs = mDurationUs;
		return OK;
	}

	{
		//Mutex::Autolock autoLock(mLock);
		if(mPlayer != NULL){
			mPlayer->control(mPlayer, CDX_CMD_GET_POSITION, (unsigned int)positionUs, 0);
		}
	}

	if(*positionUs == -1){
		*positionUs = mSeekTimeUs; //temp to fix sohuvideo bug
	}

	//LOGV("getPosition: %lld",*positionUs / 1000);

	return OK;
}

status_t CedarXPlayer::seekTo(int64_t timeUs) {

	int64_t currPositionUs;
	getPosition(&currPositionUs);

	{
		Mutex::Autolock autoLock(mLock);
		mSeekNotificationSent = false;
		LOGV("seek cmd0 to %lldms", timeUs);

		if (mFlags & CACHE_UNDERRUN) {
			mFlags &= ~CACHE_UNDERRUN;
			play_l(CDX_CMD_START_ASYNC);
		}

		mSeekTimeUs = timeUs * 1000;
		mSeeking = true;
		mFlags &= ~(AT_EOS | AUDIO_AT_EOS | VIDEO_AT_EOS);

		if (!(mFlags & PLAYING)) {
			LOGV( "seeking while paused, sending SEEK_COMPLETE notification"
						" immediately.");

			notifyListener_l(MEDIA_SEEK_COMPLETE);
			mSeekNotificationSent = true;
			return OK;
		}
	}

	mPlayer->control(mPlayer, CDX_CMD_SEEK_ASYNC, (int)timeUs, (int)(currPositionUs/1000));

	LOGV("--------- seek cmd1 to %lldms end -----------", timeUs);

	return OK;
}

void CedarXPlayer::finishAsyncPrepare_l(int err){

	LOGV("finishAsyncPrepare_l");
	if(err == -1){
		LOGE("CedarXPlayer:prepare error!");
		abortPrepare(UNKNOWN_ERROR);
		return;
	}

	mPlayer->control(mPlayer, CDX_CMD_GET_STREAM_TYPE, (unsigned int)&mStreamType, 0);
	mPlayer->control(mPlayer, CDX_CMD_GET_MEDIAINFO, (unsigned int)&mMediaInfo, 0);
	//if(mStreamType != CEDARX_STREAM_LOCALFILE) {
	mFlags &= ~CACHE_UNDERRUN;
	//}
	mVideoWidth  = mMediaInfo.mVideoInfo[0].mFrameWidth; //TODO: temp assign
	mVideoHeight = mMediaInfo.mVideoInfo[0].mFrameHeight;
	mCanSeek = mMediaInfo.mFlags & 1;
	notifyListener_l(MEDIA_SET_VIDEO_SIZE, mVideoWidth, mVideoHeight);
	mFlags &= ~(PREPARING|PREPARE_CANCELLED);
	mFlags |= PREPARED;

	if(mIsAsyncPrepare){
		notifyListener_l(MEDIA_PREPARED);
	}

	return;
}

void CedarXPlayer::finishSeek_l(int err){
	Mutex::Autolock autoLock(mLock);
	LOGV("finishSeek_l");

	if(mAudioPlayer){
		mAudioPlayer->seekTo(0);
	}
	mSeeking = false;
	if (!mSeekNotificationSent) {
		LOGV("MEDIA_SEEK_COMPLETE return");
		notifyListener_l(MEDIA_SEEK_COMPLETE);
		mSeekNotificationSent = true;
	}

	return;
}

status_t CedarXPlayer::prepareAsync() {
	Mutex::Autolock autoLock(mLock);
	if (mFlags & PREPARING) {
		return UNKNOWN_ERROR; // async prepare already pending
	}
	mFlags |= PREPARING;
	mIsAsyncPrepare = true;

	return (mPlayer->control(mPlayer, CDX_CMD_PREPARE_ASYNC, 0, 0) == 0 ? OK : UNKNOWN_ERROR);
}

status_t CedarXPlayer::prepare() {
	Mutex::Autolock autoLock(mLock);
	LOGV("prepare");
	return prepare_l();
}

status_t CedarXPlayer::prepare_l() {
	if (mFlags & PREPARED) {
	    return OK;
	}

	mIsAsyncPrepare = false;

	if(mPlayer->control(mPlayer, CDX_CMD_PREPARE, 0, 0) != 0){
		return UNKNOWN_ERROR;
	}

	finishAsyncPrepare_l(0);

	return OK;
}

void CedarXPlayer::abortPrepare(status_t err) {
	CHECK(err != OK);

	if (mIsAsyncPrepare) {
		notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
	}

	mPrepareResult = err;
	mFlags &= ~(PREPARING | PREPARE_CANCELLED);
}

status_t CedarXPlayer::suspend() {
	LOGD("suspend start");

	if (mFlags & SUSPENDING)
		return OK;

	SuspensionState *state = &mSuspensionState;
	getPosition(&state->mPositionUs);

	Mutex::Autolock autoLock(mLock);

	state->mFlags = mFlags & (PLAYING | AUTO_LOOPING | LOOPING | AT_EOS);
    state->mUri = mUri;
    state->mUriHeaders = mUriHeaders;
	mFlags |= SUSPENDING;

	if(isCedarXInitialized){
		mPlayer->control(mPlayer, CDX_CMD_STOP_ASYNC, 0, 0);
		CDXPlayer_Destroy(mPlayer);
		mPlayer = NULL;
		isCedarXInitialized = false;
	}

	pause_l(true);
	mVideoRenderer.clear();
	mVideoRenderer = NULL;
	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
	mPlayerState = PLAYER_STATE_SUSPEND;
	LOGD("suspend end");

	return OK;
}

status_t CedarXPlayer::resume() {
	LOGD("resume start");
    Mutex::Autolock autoLock(mLock);
    SuspensionState *state = &mSuspensionState;
    status_t err;

    if(mPlayer == NULL){
    	CDXPlayer_Create((void**)&mPlayer);
    	mPlayer->control(mPlayer, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXPlayerCallbackWrapper, (unsigned int)this);
    	isCedarXInitialized = true;
    }

    //mPlayer->control(mPlayer, CDX_CMD_SET_STATE, CDX_STATE_UNKOWN, 0);

    if (mIsUri) {
    	err = setDataSource(state->mUri, &state->mUriHeaders);
    } else {
        LOGW("NOT support setdatasouce non-uri currently");
    }

    mFlags = state->mFlags & (AUTO_LOOPING | LOOPING | AT_EOS);

    mFlags |= RESTORE_CONTROL_PARA;

    if (state->mFlags & PLAYING) {
        play_l(CDX_CMD_TAG_START_ASYNC);
    }
    mFlags &= ~SUSPENDING;
    //state->mPositionUs = 0;
    mPlayerState = PLAYER_STATE_RESUME;

    LOGD("resume end");

	return OK;
}

#ifndef __CHIP_VERSION_F20
status_t CedarXPlayer::setScreen(int screen) {
	mScreenID = screen;
	if(mVideoRenderer != NULL && !(mFlags & SUSPENDING)){
		LOGV("CedarX setScreen to:%d", screen);
		return mVideoRenderer->control(VIDEORENDER_CMD_SETSCREEN, screen);
	}
	return UNKNOWN_ERROR;
}

status_t CedarXPlayer::set3DMode(int mode){
	if(mVideoRenderer != NULL){
		LOGV("CedarX set3Dmode to:%d", mode);
		return mVideoRenderer->control(VIDEORENDER_CMD_SET3DMODE, mode);
	}
	return UNKNOWN_ERROR;
}

int CedarXPlayer::getMeidaPlayerState() {
    return mPlayerState;
}

int CedarXPlayer::getSubCount()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBCOUNT, (unsigned int)&tmp, 0);

	LOGV("getSubCount:%d",tmp);

    return tmp;
};

int CedarXPlayer::getSubList(MediaPlayer_SubInfo *infoList, int count)
{
	if(mPlayer == NULL){
		return -1;
	}

	if(mPlayer->control(mPlayer, CDX_CMD_GETSUBLIST, (unsigned int)infoList, count) == 0){
		return count;
	}

	return -1;
}

int CedarXPlayer::getCurSub()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETCURSUB, (unsigned int)&tmp, 0);
    return tmp;
};

status_t CedarXPlayer::switchSub(int index)
{
	if(mPlayer == NULL || mSubtitleParameter.mSubtitleIndex == index){
		return -1;
	}

	mSubtitleParameter.mSubtitleIndex = index;
	return mPlayer->control(mPlayer, CDX_CMD_SWITCHSUB, index, 0);
};

status_t CedarXPlayer::setSubGate(bool showSub)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleGate = showSub;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBGATE, showSub, 0);
};

bool CedarXPlayer::getSubGate()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBGATE, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::setSubColor(int color)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleColor = color;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBCOLOR, color, 0);
};

int CedarXPlayer::getSubColor()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBCOLOR, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::setSubFrameColor(int color)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleFrameColor = color;
    return mPlayer->control(mPlayer, CDX_CMD_SETSUBFRAMECOLOR, color, 0);
};

int CedarXPlayer::getSubFrameColor()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBFRAMECOLOR, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::setSubFontSize(int size)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleFontSize = size;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBFONTSIZE, size, 0);
};

int CedarXPlayer::getSubFontSize()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBFONTSIZE, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::setSubCharset(const char *charset)
{
	if(mPlayer == NULL){
		return -1;
	}

	//mSubtitleParameter.mSubtitleCharset = percent;
    return mPlayer->control(mPlayer, CDX_CMD_SETSUBCHARSET, (unsigned int)charset, 0);
};

status_t CedarXPlayer::getSubCharset(char *charset)
{
	if(mPlayer == NULL){
		return -1;
	}

    return mPlayer->control(mPlayer, CDX_CMD_GETSUBCHARSET, (unsigned int)charset, 0);
};

status_t CedarXPlayer::setSubPosition(int percent)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitlePosition = percent;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBPOSITION, percent, 0);
};

int CedarXPlayer::getSubPosition()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBPOSITION, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::setSubDelay(int time)
{
	if(mPlayer == NULL){
		return -1;
	}

	mSubtitleParameter.mSubtitleDelay = time;
	return mPlayer->control(mPlayer, CDX_CMD_SETSUBDELAY, time, 0);
};

int CedarXPlayer::getSubDelay()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETSUBDELAY, (unsigned int)&tmp, 0);
	return tmp;
};

int CedarXPlayer::getTrackCount()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETTRACKCOUNT, (unsigned int)&tmp, 0);
	return tmp;
};

int CedarXPlayer::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
{
	if(mPlayer == NULL){
		return -1;
	}

	if(mPlayer->control(mPlayer, CDX_CMD_GETTRACKLIST, (unsigned int)infoList, count) == 0){
		return count;
	}

	return -1;
};

int CedarXPlayer::getCurTrack()
{
	int tmp;

	if(mPlayer == NULL){
		return -1;
	}

	mPlayer->control(mPlayer, CDX_CMD_GETCURTRACK, (unsigned int)&tmp, 0);
	return tmp;
};

status_t CedarXPlayer::switchTrack(int index)
{
	if(mPlayer == NULL || mAudioTrackIndex == index){
		return -1;
	}

	mAudioTrackIndex = index;
	return mPlayer->control(mPlayer, CDX_CMD_SWITCHTRACK, index, 0);
};

status_t CedarXPlayer::setInputDimensionType(int type)
{
	if(mPlayer == NULL){
		return -1;
	}
	this->input_3d_type = type;
	return mPlayer->control(mPlayer, CDX_CMD_SET_DECODER_SOURCE_3D_FORMAT, type, 0);
};

int CedarXPlayer::getInputDimensionType()
{
	if(mPlayer == NULL){
		return -1;
	}
	return this->input_3d_type;
};

status_t CedarXPlayer::setOutputDimensionType(int type)
{
	if(mPlayer == NULL){
		return -1;
	}
	this->output_3d_type = type;
	return mPlayer->control(mPlayer, CDX_CMD_SET_DISPLAY_MODE, type, 0);
};

int CedarXPlayer::getOutputDimensionType()
{
	if(mPlayer == NULL){
		return -1;
	}
	return this->output_3d_type;
};

status_t CedarXPlayer::setAnaglaghType(int type)
{
	if(mPlayer == NULL){
		return -1;
	}
	this->anaglagh_type = type;
	return mPlayer->control(mPlayer, CDX_CMD_SET_ANAGLAGH_TYPE, type, 0);
};

int CedarXPlayer::getAnaglaghType()
{
	if(mPlayer == NULL){
		return -1;
	}
	return this->anaglagh_type;
};

status_t CedarXPlayer::getVideoEncode(char *encode)
{
    return -1;
}

int CedarXPlayer::getVideoFrameRate()
{
    return -1;
}

status_t CedarXPlayer::getAudioEncode(char *encode)
{
    return -1;
}

int CedarXPlayer::getAudioBitRate()
{
    return -1;
}

int CedarXPlayer::getAudioSampleRate()
{
    return -1;
}

#endif

uint32_t CedarXPlayer::flags() const {
	if(mCanSeek) {
		return MediaExtractor::CAN_PAUSE | MediaExtractor::CAN_SEEK  | MediaExtractor::CAN_SEEK_FORWARD  | MediaExtractor::CAN_SEEK_BACKWARD;
	}
	else {
		return MediaExtractor::CAN_PAUSE;
	}
}

int CedarXPlayer::StagefrightVideoRenderInit(int width, int height, int format, void *frame_info)
{
	LOGV("CedarXPlayer::StagefrightVideoRenderInit");
//	if (!(mVideoRendererIsPreview || mVideoRenderer == NULL)){
//		return 0;
//	}

	mDisplayWidth = width;
	mDisplayHeight = height;
	mDisplayFormat = format;

	if(mVideoWidth!=width ||  mVideoHeight!=height){
		mVideoWidth = width;
		mVideoHeight = height;
		notifyListener_l(MEDIA_SET_VIDEO_SIZE, mVideoWidth, mVideoHeight);
	}

	cedarv_picture_t * frm_inf = (cedarv_picture_t *)frame_info;
	int args[6];
	int _3d_mode;
	int display_mode;
	int format_local;
	if (mISurface != NULL) {
		//        sp<MetaData> meta = mVideoSource->getFormat();
		const char *component = "cedar_decoder";
		int32_t decodedWidth, decodedHeight;
		int32_t rotationDegrees = 0;

		decodedWidth = width;//TBD. must display width
		decodedHeight = height;//TBD. must display width

		mVideoRenderer.clear();

		// Must ensure that mVideoRenderer's destructor is actually executed
		// before creating a new one.
		IPCThreadState::self()->flushCommands();

		{
#ifndef __CHIP_VERSION_F20
			format = (format == 0x11) ? OMX_COLOR_FormatVendorMBYUV422 : OMX_COLOR_FormatVendorMBYUV420;
#endif
			// Other decoders are instantiated locally and as a consequence
			// allocate their buffers in local address space.
			mVideoRenderer = new CedarXLocalRenderer(
					false, // previewOnly
					component, (OMX_COLOR_FORMATTYPE) format, mISurface,
					//mVideoWidth, mVideoHeight, decodedWidth, decodedHeight,
					decodedWidth, decodedHeight, decodedWidth, decodedHeight,
					rotationDegrees, mScreenID);

			LOGV("CedarXLocalRenderer width:%d height:%d",decodedWidth,decodedHeight);

#ifndef __CHIP_VERSION_F20

			if(frm_inf->display_mode != CDX_DISP_MODE_ORIGINAL){

				LOGV("source 3d mode %d, output 3d mode %d, display mode %d", frm_inf->source_3d_mode, frm_inf->output_3d_mode, frm_inf->display_mode);

				if(frm_inf->source_3d_mode == CDX_3D_MODE_FS)
					_3d_mode = 1;
				else if(frm_inf->source_3d_mode == CDX_3D_MODE_LRF || frm_inf->source_3d_mode == CDX_3D_MODE_RLF)
					_3d_mode = 2;
				else if(frm_inf->source_3d_mode == CDX_3D_MODE_LRH || frm_inf->source_3d_mode == CDX_3D_MODE_RLH)
					_3d_mode = 3;
				else if(frm_inf->source_3d_mode == CDX_3D_MODE_TBH || frm_inf->source_3d_mode == CDX_3D_MODE_BTH ||
						frm_inf->source_3d_mode == CDX_3D_MODE_TBF || frm_inf->source_3d_mode == CDX_3D_MODE_BTF)
					_3d_mode = 0;
				else if(frm_inf->source_3d_mode == CDX_3D_MODE_LI)
					_3d_mode = 4;
				else
					_3d_mode = 0xff;


				format_local = (frm_inf->pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV422) ? OMX_COLOR_FormatVendorMBYUV422 : OMX_COLOR_FormatVendorMBYUV420;

				args[0] = mDisplayWidth;
				args[1] = mDisplayHeight;

				if(frm_inf->display_mode == CDX_DISP_MODE_ANAGLAGH)
				{
					if(frm_inf->source_3d_mode == CDX_3D_MODE_LRH || frm_inf->source_3d_mode == CDX_3D_MODE_RLH)
					{
						//frm_inf->width /=2;
						args[0] = mDisplayWidth/2;
					}
					else if(frm_inf->source_3d_mode == CDX_3D_MODE_TBH || frm_inf->source_3d_mode == CDX_3D_MODE_BTH)
					{
						args[1] = mDisplayHeight/2;//frm_inf->height /=2;
					}
					format_local = OMX_COLOR_FormatYUV420Planar;
					display_mode = 2;
				}
				else if(frm_inf->display_mode == CDX_DISP_MODE_3D)
				{
					display_mode = 1;
				}
				else if(frm_inf->display_mode == CDX_DISP_MODE_ORIGINAL)
				{
					display_mode = 3;
				}
				else
				{
					display_mode = 0;
				}

				args[2] = format_local;
				args[3] = _3d_mode;
				args[4] = display_mode;
				args[5] = 0;

				set3DMode((int)args);
			}
#endif
		}
	}

	return 0;
}

void CedarXPlayer::StagefrightVideoRenderData(void *frame_info, int frame_id)
{
	if(mVideoRenderer != NULL){
		cedarv_picture_t *frm_inf = (cedarv_picture_t *) frame_info;
		liboverlaypara_t overlay_para;
		int args[6];
		int _3d_mode;
		int display_mode;
		int is_mode_changed;
		int format;

		args[0] = mDisplayWidth;
		args[1] = mDisplayHeight;

		if(frm_inf->is_display_mode_changed)
		{
			if(frm_inf->source_3d_mode == CDX_3D_MODE_FS)
				_3d_mode = 1;
			else if(frm_inf->source_3d_mode == CDX_3D_MODE_LRF || frm_inf->source_3d_mode == CDX_3D_MODE_RLF)
				_3d_mode = 2;
			else if(frm_inf->source_3d_mode == CDX_3D_MODE_LRH || frm_inf->source_3d_mode == CDX_3D_MODE_RLH)
				_3d_mode = 3;
			else if(frm_inf->source_3d_mode == CDX_3D_MODE_TBH || frm_inf->source_3d_mode == CDX_3D_MODE_BTH ||
					frm_inf->source_3d_mode == CDX_3D_MODE_TBF || frm_inf->source_3d_mode == CDX_3D_MODE_BTF)
				_3d_mode = 0;
			else if(frm_inf->source_3d_mode == CDX_3D_MODE_LI)
				_3d_mode = 4;
			else
				_3d_mode = 0xFF;

			format = (frm_inf->pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV422) ? OMX_COLOR_FormatVendorMBYUV422 : OMX_COLOR_FormatVendorMBYUV420;
			is_mode_changed = 1;

			if(frm_inf->display_mode == CDX_DISP_MODE_ANAGLAGH)
			{
				if(frm_inf->source_3d_mode == CDX_3D_MODE_LRH || frm_inf->source_3d_mode == CDX_3D_MODE_RLH)
				{
					args[0] = mDisplayWidth/2;//frm_inf->width /=2;
				}
				else if(frm_inf->source_3d_mode == CDX_3D_MODE_TBH || frm_inf->source_3d_mode == CDX_3D_MODE_BTH)
				{
					args[1] = mDisplayHeight/2;//frm_inf->height /=2;
				}
				format = OMX_COLOR_FormatYUV420Planar;
				display_mode = 2;

			}
			else if(frm_inf->display_mode == CDX_DISP_MODE_3D)
			{
				display_mode = 1;
			}
			else if(frm_inf->display_mode == CDX_DISP_MODE_ORIGINAL)
			{
				display_mode = 3;
			}
			else
			{
				display_mode = 0;
			}
			LOGV("line %d, display mode changed, frm_inf->source_3d_mode %d, frm_inf->display_mode %d", __LINE__, frm_inf->source_3d_mode, frm_inf->display_mode);
			args[2] = format;
			args[3] = _3d_mode;
			args[4] = display_mode;
			args[5] = is_mode_changed;

			set3DMode((int)args);
		}
		overlay_para.bProgressiveSrc = frm_inf->is_progressive;
		overlay_para.bTopFieldFirst = frm_inf->top_field_first;
		overlay_para.pVideoInfo.frame_rate = frm_inf->frame_rate;

		if(frm_inf->display_mode == CDX_DISP_MODE_3D && frm_inf->output_3d_mode == CDX_3D_MODE_FS)
		{
			overlay_para.top_y 		= (unsigned int)frm_inf->y;
			overlay_para.top_c 		= (unsigned int)frm_inf->u;
			//overlay_para.top_v 		= (unsigned int)frm_inf->v;
			overlay_para.bottom_y	= (unsigned int)frm_inf->y2;
			overlay_para.bottom_c	= (unsigned int)frm_inf->u2;
			//overlay_para.bottom_v 	= (unsigned int)frm_inf->v2;

		}
		else if(frm_inf->display_mode == CDX_DISP_MODE_ANAGLAGH)
		{
			overlay_para.top_y 		= (unsigned int)frm_inf->u2;
			overlay_para.top_c 		= (unsigned int)frm_inf->y2;
			//overlay_para.top_v 		= (unsigned int)frm_inf->v2;
			overlay_para.bottom_y 		= (unsigned int)frm_inf->v2;
			overlay_para.bottom_c 	= 0;
			//overlay_para.bottom_v 	= 0;
		}
		else
		{
			overlay_para.top_y 		= (unsigned int)frm_inf->y;
			overlay_para.top_c 		= (unsigned int)frm_inf->u;
			//overlay_para.top_v		= (unsigned int)frm_inf->v;
			overlay_para.bottom_y 	= 0;
			overlay_para.bottom_c 	= 0;
			//overlay_para.bottom_v 	= 0;
		}
		overlay_para.number = frame_id;

		mVideoRenderer->render(&overlay_para, 0);
	}
}

int CedarXPlayer::StagefrightVideoRenderGetFrameID()
{
	if(mVideoRenderer != NULL){
#ifndef __CHIP_VERSION_F20
		return mVideoRenderer->control(VIDEORENDER_CMD_GETFRAMEID, 0);
#else
		return mVideoRenderer->getframeid();
#endif
	}
	return -1;
}

int CedarXPlayer::StagefrightAudioRenderInit(int samplerate, int channels, int format)
{
	//initial audio playback
	if (mAudioPlayer == NULL) {
		if (mAudioSink != NULL) {
			mAudioPlayer = new CedarXAudioPlayer(mAudioSink, this);
			//mAudioPlayer->setSource(mAudioSource);
			LOGV("set audio format: (%d : %d)", samplerate, channels);
			mAudioPlayer->setFormat(samplerate, channels);

			status_t err = mAudioPlayer->start(true /* sourceAlreadyStarted */);

			if (err != OK) {
				delete mAudioPlayer;
				mAudioPlayer = NULL;

				mFlags &= ~(PLAYING);

				return err;
			}

			mWaitAudioPlayback = 1;
		}
	} else {
		mAudioPlayer->resume();
	}

	return 0;
}

void CedarXPlayer::StagefrightAudioRenderExit(int immed)
{
	if(mAudioPlayer){
		delete mAudioPlayer;
		mAudioPlayer = NULL;
	}
}

int CedarXPlayer::StagefrightAudioRenderData(void* data, int len)
{
	return mAudioPlayer->render(data,len);
}

int CedarXPlayer::StagefrightAudioRenderGetSpace(void)
{
	return mAudioPlayer->getSpace();
}

int CedarXPlayer::StagefrightAudioRenderGetDelay(void)
{
	return mAudioPlayer->getLatency();
}

int CedarXPlayer::CedarXPlayerCallback(int event, void *info)
{
	int ret = 0;
	int *para = (int*)info;

	//LOGV("----------CedarXPlayerCallback event:%d info:%p\n", event, info);

	switch (event) {
	case CDX_EVENT_PLAYBACK_COMPLETE:
		mFlags &= ~PLAYING;
		mFlags |= AT_EOS;
		stop_l(); //for gallery
		break;

	case CDX_EVENT_VIDEORENDERINIT:
	    StagefrightVideoRenderInit(para[0], para[1], para[2], (void *)para[3]);
		break;

	case CDX_EVENT_VIDEORENDERDATA:
		StagefrightVideoRenderData((void*)para[0],para[1]);
		break;

	case CDX_EVENT_VIDEORENDERGETDISPID:
		*((int*)para) = StagefrightVideoRenderGetFrameID();
		break;

	case CDX_EVENT_AUDIORENDERINIT:
		StagefrightAudioRenderInit(para[0], para[1], para[2]);
		break;

	case CDX_EVENT_AUDIORENDEREXIT:
		StagefrightAudioRenderExit(0);
		break;

	case CDX_EVENT_AUDIORENDERDATA:
		ret = StagefrightAudioRenderData((void*)para[0],para[1]);
		break;

	case CDX_EVENT_AUDIORENDERGETSPACE:
		ret = StagefrightAudioRenderGetSpace();
		break;

	case CDX_EVENT_AUDIORENDERGETDELAY:
		ret = StagefrightAudioRenderGetDelay();
		break;

	case CDX_MEDIA_INFO_BUFFERING_START:
		LOGV("MEDIA_INFO_BUFFERING_START");
		notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_START);
		break;

	case CDX_MEDIA_INFO_BUFFERING_END:
		LOGV("MEDIA_INFO_BUFFERING_END ...");
		notifyListener_l(MEDIA_BUFFERING_UPDATE, 0);//clear buffer scroll
		usleep(1000);
		notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
		break;

	case CDX_MEDIA_BUFFERING_UPDATE:
	{
		int64_t positionUs;
		int progress = (int)para;

		progress = progress > 100 ? 100 : progress;
		getPosition(&positionUs);
		if(mDurationUs > 0){
			progress = (int)((positionUs + (mDurationUs - positionUs) * progress / 100) * 100 / mDurationUs);
			LOGV("MEDIA_INFO_BUFFERING_UPDATE: %d %lld", (int)progress, positionUs);
			notifyListener_l(MEDIA_BUFFERING_UPDATE, (int)progress);
		}
		break;
	}

	case CDX_EVENT_PREPARED:
		finishAsyncPrepare_l((int)para);
		break;

	case CDX_EVENT_SEEK_COMPLETE:
		finishSeek_l(0);
		break;

	case CDX_MEDIA_INFO_SRC_3D_MODE:
		{
			cdx_3d_mode_e tmp_3d_mode;
			tmp_3d_mode = *((cdx_3d_mode_e *)info);
			LOGV("source 3d mode get from parser is %d", tmp_3d_mode);
			notifyListener_l(MEDIA_INFO_SRC_3D_MODE, tmp_3d_mode);
		}
		break;

	default:
		break;
	}

	return ret;
}

extern "C" int CedarXPlayerCallbackWrapper(void *cookie, int event, void *info)
{
	return ((android::CedarXPlayer *)cookie)->CedarXPlayerCallback(event, info);
}

} // namespace android

