/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

// Proxy for media player implementations

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayerService"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>

#include <cutils/atomic.h>
#include <cutils/properties.h> // for property_get

#include <utils/misc.h>

#include <android_runtime/ActivityManager.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <gui/SurfaceTextureClient.h>
#include <utils/Errors.h>  // for status_t
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <utils/Vector.h>
#include <cutils/properties.h>

#include <media/MediaPlayerInterface.h>
#include <media/mediarecorder.h>
#include <media/MediaMetadataRetrieverInterface.h>
#include <media/Metadata.h>
#include <media/AudioTrack.h>
#include <media/MemoryLeakTrackUtil.h>
#include <media/stagefright/MediaErrors.h>

#include <system/audio.h>

#include <private/android_filesystem_config.h>

#include "MediaRecorderClient.h"
#include "MediaPlayerService.h"
#include "MetadataRetrieverClient.h"

#include "MidiFile.h"
#include "TestPlayerStub.h"
#include "StagefrightPlayer.h"
#include "nuplayer/NuPlayerDriver.h"
#include "CedarPlayer.h"
#include "CedarAPlayerWrapper.h"
#include "SimpleMediaFormatProbe.h"

#include <OMX.h>
/* add by Gary. start {{----------------------------------- */
/* save the screen info */
#define PROP_SCREEN_KEY             "media.stagefright.screen"
#define PROP_MASTER_SCREEN          "master"
#define PROP_SLAVE_SCREEN           "slave"
#define PROP_SCREEN_DEFAULT_VALUE   PROP_MASTER_SCREEN
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
#define PROP_VPP_GATE_KEY           "media.stagefright.vpp_gate"
#define PROP_ENABLE_VPP             "enable vpp"
#define PROP_DISABLE_VPP            "disable vpp"
#define PROP_VPP_GATE_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_LUMA_SHARP_KEY           "media.stagefright.luma_sharp"
#define PROP_LUMA_SHARP_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_CHROMA_SHARP_KEY           "media.stagefright.chroma_sharp"
#define PROP_CHROMA_SHARP_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_WHITE_EXTEND_KEY           "media.stagefright.white_extend"
#define PROP_WHITE_EXTEND_DEFAULT_VALUE PROP_DISABLE_VPP

#define PROP_BLACK_EXTEND_KEY           "media.stagefright.black_extend"
#define PROP_BLACK_EXTEND_DEFAULT_VALUE PROP_DISABLE_VPP

/* add by Gary. end   -----------------------------------}} */

namespace {
using android::media::Metadata;
using android::status_t;
using android::OK;
using android::BAD_VALUE;
using android::NOT_ENOUGH_DATA;
using android::Parcel;

// Max number of entries in the filter.
const int kMaxFilterSize = 64;  // I pulled that out of thin air.

// FIXME: Move all the metadata related function in the Metadata.cpp


// Unmarshall a filter from a Parcel.
// Filter format in a parcel:
//
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       number of entries (n)                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 1                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type 2                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  ....
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       metadata type n                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// @param p Parcel that should start with a filter.
// @param[out] filter On exit contains the list of metadata type to be
//                    filtered.
// @param[out] status On exit contains the status code to be returned.
// @return true if the parcel starts with a valid filter.
bool unmarshallFilter(const Parcel& p,
                      Metadata::Filter *filter,
                      status_t *status)
{
    int32_t val;
    if (p.readInt32(&val) != OK)
    {
        LOGE("Failed to read filter's length");
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    if( val > kMaxFilterSize || val < 0)
    {
        LOGE("Invalid filter len %d", val);
        *status = BAD_VALUE;
        return false;
    }

    const size_t num = val;

    filter->clear();
    filter->setCapacity(num);

    size_t size = num * sizeof(Metadata::Type);


    if (p.dataAvail() < size)
    {
        LOGE("Filter too short expected %d but got %d", size, p.dataAvail());
        *status = NOT_ENOUGH_DATA;
        return false;
    }

    const Metadata::Type *data =
            static_cast<const Metadata::Type*>(p.readInplace(size));

    if (NULL == data)
    {
        LOGE("Filter had no data");
        *status = BAD_VALUE;
        return false;
    }

    // TODO: The stl impl of vector would be more efficient here
    // because it degenerates into a memcpy on pod types. Try to
    // replace later or use stl::set.
    for (size_t i = 0; i < num; ++i)
    {
        filter->add(*data);
        ++data;
    }
    *status = OK;
    return true;
}

// @param filter Of metadata type.
// @param val To be searched.
// @return true if a match was found.
bool findMetadata(const Metadata::Filter& filter, const int32_t val)
{
    // Deal with empty and ANY right away
    if (filter.isEmpty()) return false;
    if (filter[0] == Metadata::kAny) return true;

    return filter.indexOf(val) >= 0;
}

}  // anonymous namespace


namespace android {

static bool checkPermission(const char* permissionString) {
#ifndef HAVE_ANDROID_OS
    return true;
#endif
    if (getpid() == IPCThreadState::self()->getCallingPid()) return true;
    bool ok = checkCallingPermission(String16(permissionString));
    if (!ok) LOGE("Request requires %s", permissionString);
    return ok;
}

// TODO: Temp hack until we can register players
typedef struct {
    const char *extension;
    const player_type playertype;
} extmap;
extmap FILE_EXTS [] =  {
		{".ogg", STAGEFRIGHT_PLAYER},
		{".mp3", STAGEFRIGHT_PLAYER},
		{".wav", STAGEFRIGHT_PLAYER},
		{".amr", STAGEFRIGHT_PLAYER},
		{".flac", STAGEFRIGHT_PLAYER},
		{".m4a", STAGEFRIGHT_PLAYER},

		//{".3gp", STAGEFRIGHT_PLAYER},

		//{".aac", STAGEFRIGHT_PLAYER},
        {".mid", SONIVOX_PLAYER},
        {".midi", SONIVOX_PLAYER},
        {".smf", SONIVOX_PLAYER},
        {".xmf", SONIVOX_PLAYER},
        {".imy", SONIVOX_PLAYER},
        {".rtttl", SONIVOX_PLAYER},
        {".rtx", SONIVOX_PLAYER},
        {".ota", SONIVOX_PLAYER},
        {".ape", CEDARA_PLAYER},
        {".ac3", CEDARA_PLAYER},
        {".dts", CEDARA_PLAYER},
        {".wma", CEDARA_PLAYER},
        {".aac", CEDARA_PLAYER},
        {".mp2", CEDARA_PLAYER},
        {".mp1", CEDARA_PLAYER},
};

// TODO: Find real cause of Audio/Video delay in PV framework and remove this workaround
/* static */ int MediaPlayerService::AudioOutput::mMinBufferCount = 4;
/* static */ bool MediaPlayerService::AudioOutput::mIsOnEmulator = false;

void MediaPlayerService::instantiate() {
    defaultServiceManager()->addService(
            String16("media.player"), new MediaPlayerService());
}

MediaPlayerService::MediaPlayerService()
{
    LOGV("MediaPlayerService created");
    mNextConnId = 1;

    mBatteryAudio.refCount = 0;
    for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
        mBatteryAudio.deviceOn[i] = 0;
        mBatteryAudio.lastTime[i] = 0;
        mBatteryAudio.totalTime[i] = 0;
    }
    // speaker is on by default
    mBatteryAudio.deviceOn[SPEAKER] = 1;
    /* add by Gary. start {{----------------------------------- */
    char prop_value[PROPERTY_VALUE_MAX];
    property_get(PROP_SCREEN_KEY, prop_value, PROP_SCREEN_DEFAULT_VALUE);
    LOGV("prop_value = %s", prop_value);
    String8 value( prop_value );
    if(value == PROP_MASTER_SCREEN)
        mScreen = MASTER_SCREEN;
    else
        mScreen = SLAVE_SCREEN;
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
    property_get(PROP_VPP_GATE_KEY, prop_value, PROP_VPP_GATE_DEFAULT_VALUE);
    LOGV("prop_value of PROP_VPP_GATE_KEY = %s", prop_value);
    String8 vpp_value(prop_value);
    if(vpp_value == PROP_ENABLE_VPP)
        mVppGate = true;
    else
        mVppGate = false;
    
    int int_value;
    
    property_get(PROP_LUMA_SHARP_KEY, prop_value, PROP_LUMA_SHARP_DEFAULT_VALUE);
    LOGV("prop_value of PROP_LUMA_SHARP_KEY = %s", prop_value);
    int_value = prop_value[0] - '0';
    if(int_value < 0)
        int_value = 0;
    int_value %= 5;
    mLumaSharp = int_value;
    
    property_get(PROP_CHROMA_SHARP_KEY, prop_value, PROP_CHROMA_SHARP_DEFAULT_VALUE);
    LOGV("prop_value of PROP_CHROMA_SHARP_KEY = %s", prop_value);
    int_value = prop_value[0] - '0';
    if(int_value < 0)
        int_value = 0;
    int_value %= 5;
    mChromaSharp = int_value;
    
    property_get(PROP_WHITE_EXTEND_KEY, prop_value, PROP_WHITE_EXTEND_DEFAULT_VALUE);
    LOGV("prop_value of PROP_WHITE_EXTEND_KEY = %s", prop_value);
    int_value = prop_value[0] - '0';
    if(int_value < 0)
        int_value = 0;
    int_value %= 5;
    mWhiteExtend = int_value;
    
    property_get(PROP_BLACK_EXTEND_KEY, prop_value, PROP_BLACK_EXTEND_DEFAULT_VALUE);
    LOGV("prop_value of PROP_BLACK_EXTEND_KEY = %s", prop_value);
    int_value = prop_value[0] - '0';
    if(int_value < 0)
        int_value = 0;
    int_value %= 5;
    mBlackExtend = int_value;
    /* add by Gary. end   -----------------------------------}} */
}

MediaPlayerService::~MediaPlayerService()
{
    LOGV("MediaPlayerService destroyed");
}

sp<IMediaRecorder> MediaPlayerService::createMediaRecorder(pid_t pid)
{
    sp<MediaRecorderClient> recorder = new MediaRecorderClient(this, pid);
    wp<MediaRecorderClient> w = recorder;
    Mutex::Autolock lock(mLock);
    mMediaRecorderClients.add(w);
    LOGV("Create new media recorder client from pid %d", pid);
    return recorder;
}

void MediaPlayerService::removeMediaRecorderClient(wp<MediaRecorderClient> client)
{
    Mutex::Autolock lock(mLock);
    mMediaRecorderClients.remove(client);
    LOGV("Delete media recorder client");
}

sp<IMediaMetadataRetriever> MediaPlayerService::createMetadataRetriever(pid_t pid)
{
    sp<MetadataRetrieverClient> retriever = new MetadataRetrieverClient(pid);
    LOGV("Create new media retriever from pid %d", pid);
    return retriever;
}

sp<IMediaPlayer> MediaPlayerService::create(pid_t pid, const sp<IMediaPlayerClient>& client,
        int audioSessionId)
{
    int32_t connId = android_atomic_inc(&mNextConnId);

    sp<Client> c = new Client(
            this, pid, connId, client, audioSessionId,
            IPCThreadState::self()->getCallingUid());

    LOGV("Create new client(%d) from pid %d, uid %d, ", connId, pid,
         IPCThreadState::self()->getCallingUid());
    /* add by Gary. start {{----------------------------------- */
    c->setScreen(mScreen);
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
    c->setVppGate(mVppGate);
    c->setLumaSharp(mLumaSharp);
    c->setChromaSharp(mChromaSharp);
    c->setWhiteExtend(mWhiteExtend);
    c->setBlackExtend(mBlackExtend);
    /* add by Gary. end   -----------------------------------}} */
    

    wp<Client> w = c;
    {
        Mutex::Autolock lock(mLock);
        mClients.add(w);
    }
    return c;
}

sp<IOMX> MediaPlayerService::getOMX() {
    Mutex::Autolock autoLock(mLock);

    if (mOMX.get() == NULL) {
        mOMX = new OMX;
    }

    return mOMX;
}

status_t MediaPlayerService::AudioCache::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" AudioCache\n");
    if (mHeap != 0) {
        snprintf(buffer, 255, "  heap base(%p), size(%d), flags(%d), device(%s)\n",
                mHeap->getBase(), mHeap->getSize(), mHeap->getFlags(), mHeap->getDevice());
        result.append(buffer);
    }
    snprintf(buffer, 255, "  msec per frame(%f), channel count(%d), format(%d), frame count(%ld)\n",
            mMsecsPerFrame, mChannelCount, mFormat, mFrameCount);
    result.append(buffer);
    snprintf(buffer, 255, "  sample rate(%d), size(%d), error(%d), command complete(%s)\n",
            mSampleRate, mSize, mError, mCommandComplete?"true":"false");
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t MediaPlayerService::AudioOutput::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" AudioOutput\n");
    snprintf(buffer, 255, "  stream type(%d), left - right volume(%f, %f)\n",
            mStreamType, mLeftVolume, mRightVolume);
    result.append(buffer);
    snprintf(buffer, 255, "  msec per frame(%f), latency (%d)\n",
            mMsecsPerFrame, mLatency);
    result.append(buffer);
    snprintf(buffer, 255, "  aux effect id(%d), send level (%f)\n",
            mAuxEffectId, mSendLevel);
    result.append(buffer);

    ::write(fd, result.string(), result.size());
    if (mTrack != 0) {
        mTrack->dump(fd, args);
    }
    return NO_ERROR;
}

status_t MediaPlayerService::Client::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    result.append(" Client\n");
    snprintf(buffer, 255, "  pid(%d), connId(%d), status(%d), looping(%s)\n",
            mPid, mConnId, mStatus, mLoop?"true": "false");
    result.append(buffer);
    write(fd, result.string(), result.size());
    if (mPlayer != NULL) {
        mPlayer->dump(fd, args);
    }
    if (mAudioOutput != 0) {
        mAudioOutput->dump(fd, args);
    }
    write(fd, "\n", 1);
    return NO_ERROR;
}

status_t MediaPlayerService::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump MediaPlayerService from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);
        for (int i = 0, n = mClients.size(); i < n; ++i) {
            sp<Client> c = mClients[i].promote();
            if (c != 0) c->dump(fd, args);
        }
        if (mMediaRecorderClients.size() == 0) {
                result.append(" No media recorder client\n\n");
        } else {
            for (int i = 0, n = mMediaRecorderClients.size(); i < n; ++i) {
                sp<MediaRecorderClient> c = mMediaRecorderClients[i].promote();
                if (c != 0) {
                    snprintf(buffer, 255, " MediaRecorderClient pid(%d)\n", c->mPid);
                    result.append(buffer);
                    write(fd, result.string(), result.size());
                    result = "\n";
                    c->dump(fd, args);
                }
            }
        }

        result.append(" Files opened and/or mapped:\n");
        snprintf(buffer, SIZE, "/proc/%d/maps", gettid());
        FILE *f = fopen(buffer, "r");
        if (f) {
            while (!feof(f)) {
                fgets(buffer, SIZE, f);
                if (strstr(buffer, " /mnt/sdcard/") ||
                    strstr(buffer, " /system/sounds/") ||
                    strstr(buffer, " /data/") ||
                    strstr(buffer, " /system/media/")) {
                    result.append("  ");
                    result.append(buffer);
                }
            }
            fclose(f);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        snprintf(buffer, SIZE, "/proc/%d/fd", gettid());
        DIR *d = opendir(buffer);
        if (d) {
            struct dirent *ent;
            while((ent = readdir(d)) != NULL) {
                if (strcmp(ent->d_name,".") && strcmp(ent->d_name,"..")) {
                    snprintf(buffer, SIZE, "/proc/%d/fd/%s", gettid(), ent->d_name);
                    struct stat s;
                    if (lstat(buffer, &s) == 0) {
                        if ((s.st_mode & S_IFMT) == S_IFLNK) {
                            char linkto[256];
                            int len = readlink(buffer, linkto, sizeof(linkto));
                            if(len > 0) {
                                if(len > 255) {
                                    linkto[252] = '.';
                                    linkto[253] = '.';
                                    linkto[254] = '.';
                                    linkto[255] = 0;
                                } else {
                                    linkto[len] = 0;
                                }
                                if (strstr(linkto, "/mnt/sdcard/") == linkto ||
                                    strstr(linkto, "/system/sounds/") == linkto ||
                                    strstr(linkto, "/data/") == linkto ||
                                    strstr(linkto, "/system/media/") == linkto) {
                                    result.append("  ");
                                    result.append(buffer);
                                    result.append(" -> ");
                                    result.append(linkto);
                                    result.append("\n");
                                }
                            }
                        } else {
                            result.append("  unexpected type for ");
                            result.append(buffer);
                            result.append("\n");
                        }
                    }
                }
            }
            closedir(d);
        } else {
            result.append("couldn't open ");
            result.append(buffer);
            result.append("\n");
        }

        bool dumpMem = false;
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == String16("-m")) {
                dumpMem = true;
            }
        }
        if (dumpMem) {
            dumpMemoryAddresses(fd);
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

void MediaPlayerService::removeClient(wp<Client> client)
{
    Mutex::Autolock lock(mLock);
    mClients.remove(client);
}

MediaPlayerService::Client::Client(
        const sp<MediaPlayerService>& service, pid_t pid,
        int32_t connId, const sp<IMediaPlayerClient>& client,
        int audioSessionId, uid_t uid)
{
    LOGV("Client(%d) constructor", connId);
    mPid = pid;
    mConnId = connId;
    mService = service;
    mClient = client;
    mLoop = false;
    mStatus = NO_INIT;
    mAudioSessionId = audioSessionId;
    mUID = uid;

    /* add by Gary. start {{----------------------------------- */
    mHasSurface = 0;
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-28 16:28:24 */
    /* save properties before creating the real player */
    mSubGate = true;
    mSubColor = 0xFFFFFFFF;
    mSubFrameColor = 0xFF000000;
    mSubPosition = 0;
    mSubDelay = 0;
    mSubFontSize = 24;
    strcpy(mSubCharset, CHARSET_GBK);
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    mEnableScaleMode = false;
    mScaleWidth = 0;
    mScaleHeight = 0;
    /* add by Gary. end   -----------------------------------}} */
#if CALLBACK_ANTAGONIZER
    LOGD("create Antagonizer");
    mAntagonizer = new Antagonizer(notify, this);
#endif
}

MediaPlayerService::Client::~Client()
{
    LOGV("Client(%d) destructor pid = %d", mConnId, mPid);
    mAudioOutput.clear();
    wp<Client> client(this);
    disconnect();
    mService->removeClient(client);
}

void MediaPlayerService::Client::disconnect()
{
    LOGV("disconnect(%d) from pid %d", mConnId, mPid);
    // grab local reference and clear main reference to prevent future
    // access to object
    sp<MediaPlayerBase> p;
    {
        Mutex::Autolock l(mLock);
        p = mPlayer;
    }
    mClient.clear();

    mPlayer.clear();

    // clear the notification to prevent callbacks to dead client
    // and reset the player. We assume the player will serialize
    // access to itself if necessary.
    if (p != 0) {
        p->setNotifyCallback(0, 0);
#if CALLBACK_ANTAGONIZER
        LOGD("kill Antagonizer");
        mAntagonizer->kill();
#endif
        p->reset();
    }

    disconnectNativeWindow();

    IPCThreadState::self()->flushCommands();
}

static player_type getDefaultPlayerType() {
    return CEDARX_PLAYER;
    //return STAGEFRIGHT_PLAYER;
}

player_type getPlayerType(int fd, int64_t offset, int64_t length)
{
	int r_size;
	int file_format;
    char buf[2048];
    lseek(fd, offset, SEEK_SET);
    r_size = read(fd, buf, sizeof(buf));
    lseek(fd, offset, SEEK_SET);

    long ident = *((long*)buf);

    // Ogg vorbis?
    if (ident == 0x5367674f) // 'OggS'
        return STAGEFRIGHT_PLAYER;

    // Some kind of MIDI?
    EAS_DATA_HANDLE easdata;
    if (EAS_Init(&easdata) == EAS_SUCCESS) {
        EAS_FILE locator;
        locator.path = NULL;
        locator.fd = fd;
        locator.offset = offset;
        locator.length = length;
        EAS_HANDLE  eashandle;
        if (EAS_OpenFile(easdata, &locator, &eashandle) == EAS_SUCCESS) {
            EAS_CloseFile(easdata, eashandle);
            EAS_Shutdown(easdata);
            return SONIVOX_PLAYER;
        }
        EAS_Shutdown(easdata);
    }

    file_format = audio_format_detect((unsigned char*)buf, r_size);
    LOGV("getPlayerType: %d",file_format);
    if(file_format < MEDIA_FORMAT_STAGEFRIGHT_MAX && file_format > MEDIA_FORMAT_STAGEFRIGHT_MIN){
    	LOGV("use STAGEFRIGHT_PLAYER");
    	return STAGEFRIGHT_PLAYER;
    }
    else if(file_format < MEDIA_FORMAT_CEDARA_MAX && file_format > MEDIA_FORMAT_CEDARA_MIN){
    	LOGV("use CEDARA_PLAYER");
    	return CEDARA_PLAYER;
    }

    return STAGEFRIGHT_PLAYER; //getDefaultPlayerType();
}

player_type getPlayerType(const char* url)
{
    if (TestPlayerStub::canBeUsed(url)) {
        return TEST_PLAYER;
    }
#if 0
    if (!strncasecmp("http://", url, 7)
            || !strncasecmp("https://", url, 8)) {
        size_t len = strlen(url);
        if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
            return NU_PLAYER;
        }

        if (strstr(url,"m3u8")) {
            return NU_PLAYER;
        }
    }

    if (!strncasecmp("rtsp://", url, 7)) {
        return NU_PLAYER;
    }
#endif
    // use MidiFile for MIDI extensions
    int lenURL = strlen(url);
    for (int i = 0; i < NELEM(FILE_EXTS); ++i) {
        int len = strlen(FILE_EXTS[i].extension);
        int start = lenURL - len;
        if (start > 0) {
            if (!strncasecmp(url + start, FILE_EXTS[i].extension, len)) {
                return FILE_EXTS[i].playertype;
            }
        }
    }

    return getDefaultPlayerType();
}

static sp<MediaPlayerBase> createPlayer(player_type playerType, void* cookie,
        notify_callback_f notifyFunc)
{
    sp<MediaPlayerBase> p;
    switch (playerType) {
        case CEDARX_PLAYER:
            LOGV(" create CedarXPlayer");
            p = new CedarPlayer;
            break;
        case CEDARA_PLAYER:
            LOGV(" create CedarAPlayer");
            p = new CedarAPlayerWrapper;
            break;
        case SONIVOX_PLAYER:
            LOGV(" create MidiFile");
            p = new MidiFile();
            break;
        case STAGEFRIGHT_PLAYER:
            LOGV(" create StagefrightPlayer");
            p = new StagefrightPlayer;
            break;
        case NU_PLAYER:
            LOGV(" create NuPlayer");
            p = new NuPlayerDriver;
            break;
        case TEST_PLAYER:
            LOGV("Create Test Player stub");
            p = new TestPlayerStub();
            break;
        default:
            LOGE("Unknown player type: %d", playerType);
            return NULL;
    }
    if (p != NULL) {
        if (p->initCheck() == NO_ERROR) {
            p->setNotifyCallback(cookie, notifyFunc);
        } else {
            p.clear();
        }
    }
    if (p == NULL) {
        LOGE("Failed to create player object");
    }
    return p;
}

sp<MediaPlayerBase> MediaPlayerService::Client::createPlayer(player_type playerType)
{
    // determine if we have the right player type
    sp<MediaPlayerBase> p = mPlayer;
    if ((p != NULL) && (p->playerType() != playerType)) {
        LOGV("delete player");
        p.clear();
    }
    if (p == NULL) {
        p = android::createPlayer(playerType, this, notify);
    }

    if (p != NULL) {
        p->setUID(mUID);
    }

    return p;
}

status_t MediaPlayerService::Client::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers)
{
    LOGV("setDataSource(%s)", url);
    if (url == NULL)
        return UNKNOWN_ERROR;

    if ((strncmp(url, "http://", 7) == 0) ||
        (strncmp(url, "https://", 8) == 0) ||
        (strncmp(url, "rtsp://", 7) == 0)) {
        if (!checkPermission("android.permission.INTERNET")) {
            return PERMISSION_DENIED;
        }
    }

    if (strncmp(url, "content://", 10) == 0) {
        // get a filedescriptor for the content Uri and
        // pass it to the setDataSource(fd) method

        String16 url16(url);
        int fd = android::openContentProviderFile(url16);
        if (fd < 0)
        {
            LOGE("Couldn't open fd for %s", url);
            return UNKNOWN_ERROR;
        }
        setDataSource(fd, 0, 0x7fffffffffLL); // this sets mStatus
        close(fd);
        return mStatus;
    } else {
        player_type playerType = getPlayerType(url);
        LOGV("player type = %d", playerType);

        // create the right type of player
        sp<MediaPlayerBase> p = createPlayer(playerType);
        if (p == NULL) return NO_INIT;

        if (!p->hardwareOutput()) {
            mAudioOutput = new AudioOutput(mAudioSessionId);
            static_cast<MediaPlayerInterface*>(p.get())->setAudioSink(mAudioOutput);
        }
        
        /* add by Gary. start {{----------------------------------- */
        /* 2011-9-28 16:28:24 */
        /* save properties before creating the real player */
        p->setSubGate(mSubGate);
        p->setSubColor(mSubColor);
        p->setSubFrameColor(mSubFrameColor);
        p->setSubPosition(mSubPosition);
        p->setSubDelay(mSubDelay);
        p->setSubFontSize(mSubFontSize);
        p->setSubCharset(mSubCharset);
        /* add by Gary. end   -----------------------------------}} */
        
        /* add by Gary. start {{----------------------------------- */
        /* 2011-10-9 8:54:30 */
        /* add callback for parsing 3d source */
        p->setParse3dFileCallback(this, parse3dFile);
        /* add by Gary. end   -----------------------------------}} */
        
        /* add by Gary. start {{----------------------------------- */
        /* 2011-11-14 */
        /* support scale mode */
        p->enableScaleMode(mEnableScaleMode, mScaleWidth, mScaleHeight);
        /* add by Gary. end   -----------------------------------}} */

        /* add by Gary. start {{----------------------------------- */
        /* 2011-11-30 */
        /* fix the bug about setting global attibute */
        LOGD("MediaPlayerService::Client::setDataSource() : screen = %d", mScreen);
        p->setScreen(mScreen);
        p->setVppGate(mVppGate);
        p->setLumaSharp(mLumaSharp);
        p->setChromaSharp(mChromaSharp);
        p->setWhiteExtend(mWhiteExtend);
        p->setBlackExtend(mBlackExtend);
        /* add by Gary. end   -----------------------------------}} */

        // now set data source
        LOGV(" setDataSource");
        mStatus = p->setDataSource(url, headers);
        if (mStatus == NO_ERROR) {
            mPlayer = p;
        } else {
            LOGE("  error: %d", mStatus);
        }
        return mStatus;
    }
}

status_t MediaPlayerService::Client::setDataSource(int fd, int64_t offset, int64_t length)
{
    LOGV("setDataSource fd=%d, offset=%lld, length=%lld", fd, offset, length);
    struct stat sb;
    int ret = fstat(fd, &sb);
    if (ret != 0) {
        LOGE("fstat(%d) failed: %d, %s", fd, ret, strerror(errno));
        return UNKNOWN_ERROR;
    }

    LOGV("st_dev  = %llu", sb.st_dev);
    LOGV("st_mode = %u", sb.st_mode);
    LOGV("st_uid  = %lu", sb.st_uid);
    LOGV("st_gid  = %lu", sb.st_gid);
    LOGV("st_size = %llu", sb.st_size);

    if (offset >= sb.st_size) {
        LOGE("offset error");
        ::close(fd);
        return UNKNOWN_ERROR;
    }
    if (offset + length > sb.st_size) {
        length = sb.st_size - offset;
        LOGV("calculated length = %lld", length);
    }

    player_type playerType = getPlayerType(fd, offset, length);
    LOGV("player type = %d", playerType);

    // create the right type of player
    sp<MediaPlayerBase> p = createPlayer(playerType);
    if (p == NULL) return NO_INIT;

    if (!p->hardwareOutput()) {
        mAudioOutput = new AudioOutput(mAudioSessionId);
        static_cast<MediaPlayerInterface*>(p.get())->setAudioSink(mAudioOutput);
    }

    // now set data source
    mStatus = p->setDataSource(fd, offset, length);
    if (mStatus == NO_ERROR) mPlayer = p;

    return mStatus;
}

status_t MediaPlayerService::Client::setDataSource(
        const sp<IStreamSource> &source) {
    // create the right type of player
    sp<MediaPlayerBase> p = createPlayer(NU_PLAYER);

    if (p == NULL) {
        return NO_INIT;
    }

    if (!p->hardwareOutput()) {
        mAudioOutput = new AudioOutput(mAudioSessionId);
        static_cast<MediaPlayerInterface*>(p.get())->setAudioSink(mAudioOutput);
    }
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-28 16:28:24 */
    /* save properties before creating the real player */
    p->setSubGate(mSubGate);
    p->setSubColor(mSubColor);
    p->setSubFrameColor(mSubFrameColor);
    p->setSubPosition(mSubPosition);
    p->setSubDelay(mSubDelay);
    p->setSubFontSize(mSubFontSize);
    p->setSubCharset(mSubCharset);
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-10-9 8:54:30 */
    /* add callback for parsing 3d source */
    p->setParse3dFileCallback(this, parse3dFile);
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    p->enableScaleMode(mEnableScaleMode, mScaleWidth, mScaleHeight);
    /* add by Gary. end   -----------------------------------}} */

    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-30 */
    /* fix the bug about setting global attibute */
	LOGD("MediaPlayerService::Client::setDataSource() : screen = %d", mScreen);
    p->setScreen(mScreen);
    p->setVppGate(mVppGate);
    p->setLumaSharp(mLumaSharp);
    p->setChromaSharp(mChromaSharp);
    p->setWhiteExtend(mWhiteExtend);
    p->setBlackExtend(mBlackExtend);
    /* add by Gary. end   -----------------------------------}} */

    // now set data source
    mStatus = p->setDataSource(source);

    if (mStatus == OK) {
        mPlayer = p;
    }

    return mStatus;
}

void MediaPlayerService::Client::disconnectNativeWindow() {
    if (mConnectedWindow != NULL) {
        status_t err = native_window_api_disconnect(mConnectedWindow.get(),
                NATIVE_WINDOW_API_MEDIA_HW);

        if (err != OK) {
            LOGW("native_window_api_disconnect returned an error: %s (%d)",
                    strerror(-err), err);
        }
    }
    mConnectedWindow.clear();
}

status_t MediaPlayerService::Client::setVideoSurfaceTexture(
        const sp<ISurfaceTexture>& surfaceTexture)
{
    LOGV("[%d] setVideoSurfaceTexture(%p)", mConnId, surfaceTexture.get());
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;

    sp<IBinder> binder(surfaceTexture == NULL ? NULL :
            surfaceTexture->asBinder());
    if (mConnectedWindowBinder == binder) {
        return OK;
    }

    sp<ANativeWindow> anw;
    if (surfaceTexture != NULL) {
        anw = new SurfaceTextureClient(surfaceTexture);
        status_t err = native_window_api_connect(anw.get(),
                NATIVE_WINDOW_API_MEDIA_HW);

        if (err != OK) {
            LOGE("setVideoSurfaceTexture failed: %d", err);
            // Note that we must do the reset before disconnecting from the ANW.
            // Otherwise queue/dequeue calls could be made on the disconnected
            // ANW, which may result in errors.
            reset();

            disconnectNativeWindow();

            return err;
        }
    }

    // Note that we must set the player's new SurfaceTexture before
    // disconnecting the old one.  Otherwise queue/dequeue calls could be made
    // on the disconnected ANW, which may result in errors.
    status_t err = p->setVideoSurfaceTexture(surfaceTexture);

    /* add by Gary. start {{----------------------------------- */
    mHasSurface = 1;
    /* add by Gary. end   -----------------------------------}} */

    disconnectNativeWindow();

    mConnectedWindow = anw;

    if (err == OK) {
        mConnectedWindowBinder = binder;
    } else {
        disconnectNativeWindow();
    }

    return err;
}

status_t MediaPlayerService::Client::invoke(const Parcel& request,
                                            Parcel *reply)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == NULL) return UNKNOWN_ERROR;
    return p->invoke(request, reply);
}

// This call doesn't need to access the native player.
status_t MediaPlayerService::Client::setMetadataFilter(const Parcel& filter)
{
    status_t status;
    media::Metadata::Filter allow, drop;

    if (unmarshallFilter(filter, &allow, &status) &&
        unmarshallFilter(filter, &drop, &status)) {
        Mutex::Autolock lock(mLock);

        mMetadataAllow = allow;
        mMetadataDrop = drop;
    }
    return status;
}

status_t MediaPlayerService::Client::getMetadata(
        bool update_only, bool apply_filter, Parcel *reply)
{
    sp<MediaPlayerBase> player = getPlayer();
    if (player == 0) return UNKNOWN_ERROR;

    status_t status;
    // Placeholder for the return code, updated by the caller.
    reply->writeInt32(-1);

    media::Metadata::Filter ids;

    // We don't block notifications while we fetch the data. We clear
    // mMetadataUpdated first so we don't lose notifications happening
    // during the rest of this call.
    {
        Mutex::Autolock lock(mLock);
        if (update_only) {
            ids = mMetadataUpdated;
        }
        mMetadataUpdated.clear();
    }

    media::Metadata metadata(reply);

    metadata.appendHeader();
    status = player->getMetadata(ids, reply);

    if (status != OK) {
        metadata.resetParcel();
        LOGE("getMetadata failed %d", status);
        return status;
    }

    // FIXME: Implement filtering on the result. Not critical since
    // filtering takes place on the update notifications already. This
    // would be when all the metadata are fetch and a filter is set.

    // Everything is fine, update the metadata length.
    metadata.updateLength();
    return OK;
}

status_t MediaPlayerService::Client::prepareAsync()
{
    LOGV("[%d] prepareAsync", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    status_t ret = p->prepareAsync();
#if CALLBACK_ANTAGONIZER
    LOGD("start Antagonizer");
    if (ret == NO_ERROR) mAntagonizer->start();
#endif
    return ret;
}

status_t MediaPlayerService::Client::start()
{
    LOGV("[%d] start", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    p->setLooping(mLoop);
    return p->start();
}

status_t MediaPlayerService::Client::stop()
{
    LOGV("[%d] stop", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->stop();
}

status_t MediaPlayerService::Client::pause()
{
    LOGV("[%d] pause", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->pause();
}

status_t MediaPlayerService::Client::isPlaying(bool* state)
{
    *state = false;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    *state = p->isPlaying();
    LOGV("[%d] isPlaying: %d", mConnId, *state);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::getCurrentPosition(int *msec)
{
    LOGV("getCurrentPosition");
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    status_t ret = p->getCurrentPosition(msec);
    if (ret == NO_ERROR) {
        LOGV("[%d] getCurrentPosition = %d", mConnId, *msec);
    } else {
        LOGE("getCurrentPosition returned %d", ret);
    }
    return ret;
}

status_t MediaPlayerService::Client::getDuration(int *msec)
{
    LOGV("getDuration");
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    status_t ret = p->getDuration(msec);
    if (ret == NO_ERROR) {
        LOGV("[%d] getDuration = %d", mConnId, *msec);
    } else {
        LOGE("getDuration returned %d", ret);
    }
    return ret;
}

status_t MediaPlayerService::Client::seekTo(int msec)
{
    LOGV("[%d] seekTo(%d)", mConnId, msec);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->seekTo(msec);
}

status_t MediaPlayerService::Client::reset()
{
    LOGV("[%d] reset", mConnId);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->reset();
}

status_t MediaPlayerService::Client::setAudioStreamType(int type)
{
    LOGV("[%d] setAudioStreamType(%d)", mConnId, type);
    // TODO: for hardware output, call player instead
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) mAudioOutput->setAudioStreamType(type);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setLooping(int loop)
{
    LOGV("[%d] setLooping(%d)", mConnId, loop);
    mLoop = loop;
    sp<MediaPlayerBase> p = getPlayer();
    if (p != 0) return p->setLooping(loop);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setVolume(float leftVolume, float rightVolume)
{
    LOGV("[%d] setVolume(%f, %f)", mConnId, leftVolume, rightVolume);
    // TODO: for hardware output, call player instead
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) mAudioOutput->setVolume(leftVolume, rightVolume);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setAuxEffectSendLevel(float level)
{
    LOGV("[%d] setAuxEffectSendLevel(%f)", mConnId, level);
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) return mAudioOutput->setAuxEffectSendLevel(level);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::attachAuxEffect(int effectId)
{
    LOGV("[%d] attachAuxEffect(%d)", mConnId, effectId);
    Mutex::Autolock l(mLock);
    if (mAudioOutput != 0) return mAudioOutput->attachAuxEffect(effectId);
    return NO_ERROR;
}

status_t MediaPlayerService::Client::setParameter(int key, const Parcel &request) {
    LOGV("[%d] setParameter(%d)", mConnId, key);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->setParameter(key, request);
}

status_t MediaPlayerService::Client::getParameter(int key, Parcel *reply) {
    LOGV("[%d] getParameter(%d)", mConnId, key);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) return UNKNOWN_ERROR;
    return p->getParameter(key, reply);
}

/* add by Gary. start {{----------------------------------- */
status_t MediaPlayerService::setScreen(int screen)
{
    LOGV("setScreen(%d)", screen);
    if( screen != MASTER_SCREEN && screen != SLAVE_SCREEN )
        return BAD_VALUE;
    if( screen == mScreen )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setScreen(screen);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mScreen = screen;
    
    if(mScreen == MASTER_SCREEN)
        property_set(PROP_SCREEN_KEY, PROP_MASTER_SCREEN);
    else
        property_set(PROP_SCREEN_KEY, PROP_SLAVE_SCREEN);
    char prop_value[PROPERTY_VALUE_MAX];
    property_get(PROP_SCREEN_KEY, prop_value, "no screen");
    LOGV("prop_value = %s", prop_value);
        
    return ret;
}

status_t MediaPlayerService::getScreen(int *screen)
{
    LOGV("get Screen");
    if( screen == NULL )
        return BAD_VALUE;
        
    *screen = mScreen;
    return OK;
}

status_t MediaPlayerService::Client::setScreen(int screen)
{
    LOGV("[%d] setScreen(%d)", mConnId, screen);
    mScreen = screen;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setScreen(screen);
}

status_t MediaPlayerService::isPlayingVideo(int *playing)
{
    status_t ret = OK;
    *playing = 0;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            c->isPlayingVideo(playing);
            if( *playing == 1 )
                return OK;
        }
    }
    
    return ret;
}

status_t MediaPlayerService::Client::isPlayingVideo(int *playing)
{
	sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) {
    	*playing = 0;
        return OK;
    }
    *playing = mHasSurface && (p->getMeidaPlayerState() != PLAYER_STATE_SUSPEND);
    return OK;
}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-9-15 15:41:36 */
/* expend interfaces about subtitle, track and so on */
int MediaPlayerService::Client::getSubCount()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getSubCount();
}


int MediaPlayerService::Client::getSubList(MediaPlayer_SubInfo *infoList, int count)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getSubList((MediaPlayer_SubInfo *)infoList, count);
}

int MediaPlayerService::Client::getCurSub()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getCurSub();
}

status_t MediaPlayerService::Client::switchSub(int index)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->switchSub(index);
}

status_t MediaPlayerService::Client::setSubGate(bool showSub)
{
    mSubGate = showSub;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubGate(showSub);
}

bool MediaPlayerService::Client::getSubGate()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return true;
    return p->getSubGate();
}

status_t MediaPlayerService::Client::setSubColor(int color)
{
    mSubColor = color;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubColor(color);
}

int MediaPlayerService::Client::getSubColor()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return 0xFFFFFFFF;
    return p->getSubColor();
}

status_t MediaPlayerService::Client::setSubFrameColor(int color)
{
    mSubFrameColor = color;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubFrameColor(color);
}

int MediaPlayerService::Client::getSubFrameColor()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return 0xFFFFFFFF;
    return p->getSubFrameColor();
}

status_t MediaPlayerService::Client::setSubFontSize(int size)
{
    mSubFontSize = size;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubFontSize(size);
}

int MediaPlayerService::Client::getSubFontSize()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getSubFontSize();
}

status_t MediaPlayerService::Client::setSubCharset(const char *charset)
{
    strcpy(mSubCharset, charset);
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubCharset(charset);
}

status_t MediaPlayerService::Client::getSubCharset(char *charset)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->getSubCharset(charset);
}

status_t MediaPlayerService::Client::setSubPosition(int percent)
{
    mSubPosition = percent;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubPosition(percent);
}

int MediaPlayerService::Client::getSubPosition()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getSubPosition();
}

status_t MediaPlayerService::Client::setSubDelay(int time)
{
    mSubDelay = time;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return OK;
    return p->setSubDelay(time);
}

int MediaPlayerService::Client::getSubDelay()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return 0;
    return p->getSubDelay();
}

int MediaPlayerService::Client::getTrackCount()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getTrackCount();
}

int MediaPlayerService::Client::getTrackList(MediaPlayer_TrackInfo *infoList, int count)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getTrackList((MediaPlayer_TrackInfo *)infoList, count);
}

int MediaPlayerService::Client::getCurTrack()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getCurTrack();
}

status_t MediaPlayerService::Client::switchTrack(int index)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->switchTrack(index);
}

status_t MediaPlayerService::Client::setInputDimensionType(int type)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setInputDimensionType(type);
}

int MediaPlayerService::Client::getInputDimensionType()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getInputDimensionType();
}

status_t MediaPlayerService::Client::setOutputDimensionType(int type)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setOutputDimensionType(type);
}

int MediaPlayerService::Client::getOutputDimensionType()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getOutputDimensionType();
}

status_t MediaPlayerService::Client::setAnaglaghType(int type)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setAnaglaghType(type);
}

int MediaPlayerService::Client::getAnaglaghType()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return -1;
    return p->getAnaglaghType();
}

status_t MediaPlayerService::Client::getVideoEncode(char *encode)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0)
        return UNKNOWN_ERROR;
    return p->getVideoEncode(encode);
}

int MediaPlayerService::Client::getVideoFrameRate()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0)
        return UNKNOWN_ERROR;
    return p->getVideoFrameRate();
}

status_t MediaPlayerService::Client::getAudioEncode(char *encode)
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0)
        return UNKNOWN_ERROR;
    return p->getAudioEncode(encode);
}

int MediaPlayerService::Client::getAudioBitRate()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0)
        return UNKNOWN_ERROR;
    return p->getAudioBitRate();
}

int MediaPlayerService::Client::getAudioSampleRate()
{
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0)
        return UNKNOWN_ERROR;
    return p->getAudioSampleRate();
}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-10-9 8:54:30 */
/* add callback for parsing 3d source */
void MediaPlayerService::Client::parse3dFile(void* cookie, int type)
{
    Client* client = static_cast<Client*>(cookie);

    LOGV("Client::parse3dFile: type = %d", type);
    client->mClient->parse3dFile(type);
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support scale mode */
status_t MediaPlayerService::Client::enableScaleMode(bool enable, int width, int height)
{
    mEnableScaleMode = enable;
    mScaleWidth = width;
    mScaleHeight = height;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->enableScaleMode(enable, width, height);
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
status_t MediaPlayerService::setVppGate(bool enableVpp)
{
    if( enableVpp == mVppGate )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setVppGate(enableVpp);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mVppGate = enableVpp;
    
    if(mVppGate)
        property_set(PROP_VPP_GATE_KEY, PROP_ENABLE_VPP);
    else
        property_set(PROP_VPP_GATE_KEY, PROP_DISABLE_VPP);
    char prop_value[PROPERTY_VALUE_MAX];
    property_get(PROP_VPP_GATE_KEY, prop_value, "no enableVpp");
    LOGV("prop_value of PROP_VPP_GATE_KEY = %s", prop_value);
        
    return ret;
}

bool MediaPlayerService::getVppGate()
{
    return mVppGate;
}

status_t MediaPlayerService::Client::setVppGate(bool enableVpp)
{
    mVppGate = enableVpp;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setVppGate(enableVpp);
}

status_t MediaPlayerService::setLumaSharp(int value)
{
    if(value < 0)
        return BAD_VALUE;
        
    value %= 5;
    
    if( value == mLumaSharp )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setLumaSharp(value);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mLumaSharp = value;
    
    char prop_value[2] = "\0";
    prop_value[0] = "01234"[value];    
    property_set(PROP_LUMA_SHARP_KEY, prop_value);
    
    char prop_value2[PROPERTY_VALUE_MAX];
    property_get(PROP_LUMA_SHARP_KEY, prop_value2, "no proper LUMA_SHARP");
    LOGV("prop_value of PROP_LUMA_SHARP_KEY = %s", prop_value2);
        
    return ret;
}

int MediaPlayerService::getLumaSharp()
{
    return mLumaSharp;
}

status_t MediaPlayerService::Client::setLumaSharp(int value)
{
    mLumaSharp = value;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setLumaSharp(value);
}

status_t MediaPlayerService::setChromaSharp(int value)
{
    if(value < 0)
        return BAD_VALUE;
        
    value %= 5;
    
    if( value == mChromaSharp )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setChromaSharp(value);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mChromaSharp = value;
    
    char prop_value[2] = "\0";
    prop_value[0] = "01234"[value];    
    property_set(PROP_CHROMA_SHARP_KEY, prop_value);
    
    char prop_value2[PROPERTY_VALUE_MAX];
    property_get(PROP_CHROMA_SHARP_KEY, prop_value2, "no proper CHROMA_SHARP");
    LOGV("prop_value of PROP_CHROMA_SHARP_KEY = %s", prop_value2);
        
    return ret;
}

int MediaPlayerService::getChromaSharp()
{
    return mChromaSharp;
}

status_t MediaPlayerService::Client::setChromaSharp(int value)
{
    mChromaSharp = value;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setChromaSharp(value);
}

status_t MediaPlayerService::setWhiteExtend(int value)
{
    if(value < 0)
        return BAD_VALUE;
        
    value %= 5;
    
    if( value == mWhiteExtend )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setWhiteExtend(value);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mWhiteExtend = value;
    
    char prop_value[2] = "\0";
    prop_value[0] = "01234"[value];    
    property_set(PROP_WHITE_EXTEND_KEY, prop_value);
    
    char prop_value2[PROPERTY_VALUE_MAX];
    property_get(PROP_WHITE_EXTEND_KEY, prop_value2, "no proper WHITE_EXTEND");
    LOGV("prop_value of PROP_WHITE_EXTEND_KEY = %s", prop_value2);
        
    return ret;
}

int MediaPlayerService::getWhiteExtend()
{
    return mWhiteExtend;
}

status_t MediaPlayerService::Client::setWhiteExtend(int value)
{
    mWhiteExtend = value;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setWhiteExtend(value);
}

status_t MediaPlayerService::setBlackExtend(int value)
{
    if(value < 0)
        return BAD_VALUE;
        
    value %= 5;
    
    if( value == mBlackExtend )
        return OK;
        
    status_t ret = OK;
    for (int i = 0, n = mClients.size(); i < n; ++i) {
        sp<Client> c = mClients[i].promote();
        if (c != 0) {
            status_t temp = c->setBlackExtend(value);
            if( temp != OK )
                ret = temp;
        }
    }
    
    mBlackExtend = value;
    
    char prop_value[2] = "\0";
    prop_value[0] = "01234"[value];
    property_set(PROP_BLACK_EXTEND_KEY, prop_value);
    
    char prop_value2[PROPERTY_VALUE_MAX];
    property_get(PROP_BLACK_EXTEND_KEY, prop_value2, "no proper BLACK_EXTEND");
    LOGV("prop_value of PROP_BLACK_EXTEND_KEY = %s", prop_value2);
        
    return ret;
}

int MediaPlayerService::getBlackExtend()
{
    return mBlackExtend;
}

status_t MediaPlayerService::Client::setBlackExtend(int value)
{
    mBlackExtend = value;
    sp<MediaPlayerBase> p = getPlayer();
    if (p == 0) 
        return UNKNOWN_ERROR;
    return p->setBlackExtend(value);
}

/* add by Gary. end   -----------------------------------}} */
void MediaPlayerService::Client::notify(
        void* cookie, int msg, int ext1, int ext2, const Parcel *obj)
{
    Client* client = static_cast<Client*>(cookie);

    if (MEDIA_INFO == msg &&
        MEDIA_INFO_METADATA_UPDATE == ext1) {
        const media::Metadata::Type metadata_type = ext2;

        if(client->shouldDropMetadata(metadata_type)) {
            return;
        }

        // Update the list of metadata that have changed. getMetadata
        // also access mMetadataUpdated and clears it.
        client->addNewMetadataUpdate(metadata_type);
    }
    LOGV("[%d] notify (%p, %d, %d, %d)", client->mConnId, cookie, msg, ext1, ext2);
    client->mClient->notify(msg, ext1, ext2, obj);
}


bool MediaPlayerService::Client::shouldDropMetadata(media::Metadata::Type code) const
{
    Mutex::Autolock lock(mLock);

    if (findMetadata(mMetadataDrop, code)) {
        return true;
    }

    if (mMetadataAllow.isEmpty() || findMetadata(mMetadataAllow, code)) {
        return false;
    } else {
        return true;
    }
}


void MediaPlayerService::Client::addNewMetadataUpdate(media::Metadata::Type metadata_type) {
    Mutex::Autolock lock(mLock);
    if (mMetadataUpdated.indexOf(metadata_type) < 0) {
        mMetadataUpdated.add(metadata_type);
    }
}

#if CALLBACK_ANTAGONIZER
const int Antagonizer::interval = 10000; // 10 msecs

Antagonizer::Antagonizer(notify_callback_f cb, void* client) :
    mExit(false), mActive(false), mClient(client), mCb(cb)
{
    createThread(callbackThread, this);
}

void Antagonizer::kill()
{
    Mutex::Autolock _l(mLock);
    mActive = false;
    mExit = true;
    mCondition.wait(mLock);
}

int Antagonizer::callbackThread(void* user)
{
    LOGD("Antagonizer started");
    Antagonizer* p = reinterpret_cast<Antagonizer*>(user);
    while (!p->mExit) {
        if (p->mActive) {
            LOGV("send event");
            p->mCb(p->mClient, 0, 0, 0);
        }
        usleep(interval);
    }
    Mutex::Autolock _l(p->mLock);
    p->mCondition.signal();
    LOGD("Antagonizer stopped");
    return 0;
}
#endif

static size_t kDefaultHeapSize = 1024 * 1024; // 1MB

sp<IMemory> MediaPlayerService::decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, int* pFormat)
{
    LOGV("decode(%s)", url);
    sp<MemoryBase> mem;
    sp<MediaPlayerBase> player;

    // Protect our precious, precious DRMd ringtones by only allowing
    // decoding of http, but not filesystem paths or content Uris.
    // If the application wants to decode those, it should open a
    // filedescriptor for them and use that.
    if (url != NULL && strncmp(url, "http://", 7) != 0) {
        LOGD("Can't decode %s by path, use filedescriptor instead", url);
        return mem;
    }

    player_type playerType = getPlayerType(url);
    LOGV("player type = %d", playerType);

    // create the right type of player
    sp<AudioCache> cache = new AudioCache(url);
    player = android::createPlayer(playerType, cache.get(), cache->notify);
    if (player == NULL) goto Exit;
    if (player->hardwareOutput()) goto Exit;

    static_cast<MediaPlayerInterface*>(player.get())->setAudioSink(cache);

    // set data source
    if (player->setDataSource(url) != NO_ERROR) goto Exit;

    LOGV("prepare");
    player->prepareAsync();

    LOGV("wait for prepare");
    if (cache->wait() != NO_ERROR) goto Exit;

    LOGV("start");
    player->start();

    LOGV("wait for playback complete");
    cache->wait();
    // in case of error, return what was successfully decoded.
    if (cache->size() == 0) {
        goto Exit;
    }

    mem = new MemoryBase(cache->getHeap(), 0, cache->size());
    *pSampleRate = cache->sampleRate();
    *pNumChannels = cache->channelCount();
    *pFormat = (int)cache->format();
    LOGV("return memory @ %p, sampleRate=%u, channelCount = %d, format = %d", mem->pointer(), *pSampleRate, *pNumChannels, *pFormat);

Exit:
    if (player != 0) player->reset();
    return mem;
}

sp<IMemory> MediaPlayerService::decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, int* pFormat)
{
    LOGV("decode(%d, %lld, %lld)", fd, offset, length);
    sp<MemoryBase> mem;
    sp<MediaPlayerBase> player;

    player_type playerType = getPlayerType(fd, offset, length);
    LOGV("player type = %d", playerType);

    // create the right type of player
    sp<AudioCache> cache = new AudioCache("decode_fd");
    player = android::createPlayer(playerType, cache.get(), cache->notify);
    if (player == NULL) goto Exit;
    if (player->hardwareOutput()) goto Exit;

    static_cast<MediaPlayerInterface*>(player.get())->setAudioSink(cache);

    // set data source
    if (player->setDataSource(fd, offset, length) != NO_ERROR) goto Exit;

    LOGV("prepare");
    player->prepareAsync();

    LOGV("wait for prepare");
    if (cache->wait() != NO_ERROR) goto Exit;

    LOGV("start");
    player->start();

    LOGV("wait for playback complete");
    cache->wait();
    // in case of error, return what was successfully decoded.
    if (cache->size() == 0) {
        goto Exit;
    }

    mem = new MemoryBase(cache->getHeap(), 0, cache->size());
    *pSampleRate = cache->sampleRate();
    *pNumChannels = cache->channelCount();
    *pFormat = cache->format();
    LOGV("return memory @ %p, sampleRate=%u, channelCount = %d, format = %d", mem->pointer(), *pSampleRate, *pNumChannels, *pFormat);

Exit:
    if (player != 0) player->reset();
    ::close(fd);
    return mem;
}


#undef LOG_TAG
#define LOG_TAG "AudioSink"
MediaPlayerService::AudioOutput::AudioOutput(int sessionId)
    : mCallback(NULL),
      mCallbackCookie(NULL),
      mSessionId(sessionId) {
    LOGV("AudioOutput(%d)", sessionId);
    mTrack = 0;
    mStreamType = AUDIO_STREAM_MUSIC;
    mLeftVolume = 1.0;
    mRightVolume = 1.0;
    mLatency = 0;
    mMsecsPerFrame = 0;
    mAuxEffectId = 0;
    mSendLevel = 0.0;
    setMinBufferCount();
}

MediaPlayerService::AudioOutput::~AudioOutput()
{
    close();
}

void MediaPlayerService::AudioOutput::setMinBufferCount()
{
    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.kernel.qemu", value, 0)) {
        mIsOnEmulator = true;
        mMinBufferCount = 12;  // to prevent systematic buffer underrun for emulator
    }
}

bool MediaPlayerService::AudioOutput::isOnEmulator()
{
    setMinBufferCount();
    return mIsOnEmulator;
}

int MediaPlayerService::AudioOutput::getMinBufferCount()
{
    setMinBufferCount();
    return mMinBufferCount;
}

ssize_t MediaPlayerService::AudioOutput::bufferSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount() * frameSize();
}

ssize_t MediaPlayerService::AudioOutput::frameCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount();
}

ssize_t MediaPlayerService::AudioOutput::channelCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->channelCount();
}

ssize_t MediaPlayerService::AudioOutput::frameSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameSize();
}

uint32_t MediaPlayerService::AudioOutput::latency () const
{
    return mLatency;
}

float MediaPlayerService::AudioOutput::msecsPerFrame() const
{
    return mMsecsPerFrame;
}

status_t MediaPlayerService::AudioOutput::getPosition(uint32_t *position)
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->getPosition(position);
}

status_t MediaPlayerService::AudioOutput::open(
        uint32_t sampleRate, int channelCount, int format, int bufferCount,
        AudioCallback cb, void *cookie)
{
    mCallback = cb;
    mCallbackCookie = cookie;

    // Check argument "bufferCount" against the mininum buffer count
    if (bufferCount < mMinBufferCount) {
        LOGD("bufferCount (%d) is too small and increased to %d", bufferCount, mMinBufferCount);
        bufferCount = mMinBufferCount;

    }
    LOGV("open(%u, %d, %d, %d, %d)", sampleRate, channelCount, format, bufferCount,mSessionId);
    if (mTrack) close();
    int afSampleRate;
    int afFrameCount;
    int frameCount;

    if (AudioSystem::getOutputFrameCount(&afFrameCount, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }
    if (AudioSystem::getOutputSamplingRate(&afSampleRate, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }

    frameCount = (sampleRate*afFrameCount*bufferCount)/afSampleRate;

    AudioTrack *t;
    if (mCallback != NULL) {
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AUDIO_CHANNEL_OUT_STEREO : AUDIO_CHANNEL_OUT_MONO,
                frameCount,
                0 /* flags */,
                CallbackWrapper,
                this,
                0,
                mSessionId);
    } else {
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AUDIO_CHANNEL_OUT_STEREO : AUDIO_CHANNEL_OUT_MONO,
                frameCount,
                0,
                NULL,
                NULL,
                0,
                mSessionId);
    }

    if ((t == 0) || (t->initCheck() != NO_ERROR)) {
        LOGE("Unable to create audio track");
        delete t;
        return NO_INIT;
    }

    LOGV("setVolume");
    t->setVolume(mLeftVolume, mRightVolume);

    mMsecsPerFrame = 1.e3 / (float) sampleRate;
    mLatency = t->latency();
    mTrack = t;

    t->setAuxEffectSendLevel(mSendLevel);
    return t->attachAuxEffect(mAuxEffectId);;
}

void MediaPlayerService::AudioOutput::start()
{
    LOGV("start");
    if (mTrack) {
        mTrack->setVolume(mLeftVolume, mRightVolume);
        mTrack->setAuxEffectSendLevel(mSendLevel);
        mTrack->start();
    }
}



ssize_t MediaPlayerService::AudioOutput::write(const void* buffer, size_t size)
{
    LOG_FATAL_IF(mCallback != NULL, "Don't call write if supplying a callback.");

    //LOGV("write(%p, %u)", buffer, size);
    if (mTrack) {
        ssize_t ret = mTrack->write(buffer, size);
        return ret;
    }
    return NO_INIT;
}

void MediaPlayerService::AudioOutput::stop()
{
    LOGV("stop");
    if (mTrack) mTrack->stop();
}

void MediaPlayerService::AudioOutput::flush()
{
    LOGV("flush");
    if (mTrack) mTrack->flush();
}

void MediaPlayerService::AudioOutput::pause()
{
    LOGV("pause");
    if (mTrack) mTrack->pause();
}

void MediaPlayerService::AudioOutput::close()
{
    LOGV("close");
    delete mTrack;
    mTrack = 0;
}

void MediaPlayerService::AudioOutput::setVolume(float left, float right)
{
    LOGV("setVolume(%f, %f)", left, right);
    mLeftVolume = left;
    mRightVolume = right;
    if (mTrack) {
        mTrack->setVolume(left, right);
    }
}

status_t MediaPlayerService::AudioOutput::setAuxEffectSendLevel(float level)
{
    LOGV("setAuxEffectSendLevel(%f)", level);
    mSendLevel = level;
    if (mTrack) {
        return mTrack->setAuxEffectSendLevel(level);
    }
    return NO_ERROR;
}

status_t MediaPlayerService::AudioOutput::attachAuxEffect(int effectId)
{
    LOGV("attachAuxEffect(%d)", effectId);
    mAuxEffectId = effectId;
    if (mTrack) {
        return mTrack->attachAuxEffect(effectId);
    }
    return NO_ERROR;
}

// static
void MediaPlayerService::AudioOutput::CallbackWrapper(
        int event, void *cookie, void *info) {
    //LOGV("callbackwrapper");
    if (event != AudioTrack::EVENT_MORE_DATA) {
        return;
    }

    AudioOutput *me = (AudioOutput *)cookie;
    AudioTrack::Buffer *buffer = (AudioTrack::Buffer *)info;

    size_t actualSize = (*me->mCallback)(
            me, buffer->raw, buffer->size, me->mCallbackCookie);

    if (actualSize == 0 && buffer->size > 0) {
        // We've reached EOS but the audio track is not stopped yet,
        // keep playing silence.

        memset(buffer->raw, 0, buffer->size);
        actualSize = buffer->size;
    }

    buffer->size = actualSize;
}

int MediaPlayerService::AudioOutput::getSessionId()
{
    return mSessionId;
}

#undef LOG_TAG
#define LOG_TAG "AudioCache"
MediaPlayerService::AudioCache::AudioCache(const char* name) :
    mChannelCount(0), mFrameCount(1024), mSampleRate(0), mSize(0),
    mError(NO_ERROR), mCommandComplete(false)
{
    // create ashmem heap
    mHeap = new MemoryHeapBase(kDefaultHeapSize, 0, name);
}

uint32_t MediaPlayerService::AudioCache::latency () const
{
    return 0;
}

float MediaPlayerService::AudioCache::msecsPerFrame() const
{
    return mMsecsPerFrame;
}

status_t MediaPlayerService::AudioCache::getPosition(uint32_t *position)
{
    if (position == 0) return BAD_VALUE;
    *position = mSize;
    return NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

struct CallbackThread : public Thread {
    CallbackThread(const wp<MediaPlayerBase::AudioSink> &sink,
                   MediaPlayerBase::AudioSink::AudioCallback cb,
                   void *cookie);

protected:
    virtual ~CallbackThread();

    virtual bool threadLoop();

private:
    wp<MediaPlayerBase::AudioSink> mSink;
    MediaPlayerBase::AudioSink::AudioCallback mCallback;
    void *mCookie;
    void *mBuffer;
    size_t mBufferSize;

    CallbackThread(const CallbackThread &);
    CallbackThread &operator=(const CallbackThread &);
};

CallbackThread::CallbackThread(
        const wp<MediaPlayerBase::AudioSink> &sink,
        MediaPlayerBase::AudioSink::AudioCallback cb,
        void *cookie)
    : mSink(sink),
      mCallback(cb),
      mCookie(cookie),
      mBuffer(NULL),
      mBufferSize(0) {
}

CallbackThread::~CallbackThread() {
    if (mBuffer) {
        free(mBuffer);
        mBuffer = NULL;
    }
}

bool CallbackThread::threadLoop() {
    sp<MediaPlayerBase::AudioSink> sink = mSink.promote();
    if (sink == NULL) {
        return false;
    }

    if (mBuffer == NULL) {
        mBufferSize = sink->bufferSize();
        mBuffer = malloc(mBufferSize);
    }

    size_t actualSize =
        (*mCallback)(sink.get(), mBuffer, mBufferSize, mCookie);

    if (actualSize > 0) {
        sink->write(mBuffer, actualSize);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

status_t MediaPlayerService::AudioCache::open(
        uint32_t sampleRate, int channelCount, int format, int bufferCount,
        AudioCallback cb, void *cookie)
{
    LOGV("open(%u, %d, %d, %d)", sampleRate, channelCount, format, bufferCount);
    if (mHeap->getHeapID() < 0) {
        return NO_INIT;
    }

    mSampleRate = sampleRate;
    mChannelCount = (uint16_t)channelCount;
    mFormat = (uint16_t)format;
    mMsecsPerFrame = 1.e3 / (float) sampleRate;

    if (cb != NULL) {
        mCallbackThread = new CallbackThread(this, cb, cookie);
    }
    return NO_ERROR;
}

void MediaPlayerService::AudioCache::start() {
    if (mCallbackThread != NULL) {
        mCallbackThread->run("AudioCache callback");
    }
}

void MediaPlayerService::AudioCache::stop() {
    if (mCallbackThread != NULL) {
        mCallbackThread->requestExitAndWait();
    }
}

ssize_t MediaPlayerService::AudioCache::write(const void* buffer, size_t size)
{
    LOGV("write(%p, %u)", buffer, size);
    if ((buffer == 0) || (size == 0)) return size;

    uint8_t* p = static_cast<uint8_t*>(mHeap->getBase());
    if (p == NULL) return NO_INIT;
    p += mSize;
    LOGV("memcpy(%p, %p, %u)", p, buffer, size);
    if (mSize + size > mHeap->getSize()) {
        LOGE("Heap size overflow! req size: %d, max size: %d", (mSize + size), mHeap->getSize());
        size = mHeap->getSize() - mSize;
    }
    memcpy(p, buffer, size);
    mSize += size;
    return size;
}

// call with lock held
status_t MediaPlayerService::AudioCache::wait()
{
    Mutex::Autolock lock(mLock);
    while (!mCommandComplete) {
        mSignal.wait(mLock);
    }
    mCommandComplete = false;

    if (mError == NO_ERROR) {
        LOGV("wait - success");
    } else {
        LOGV("wait - error");
    }
    return mError;
}

void MediaPlayerService::AudioCache::notify(
        void* cookie, int msg, int ext1, int ext2, const Parcel *obj)
{
    LOGV("notify(%p, %d, %d, %d)", cookie, msg, ext1, ext2);
    AudioCache* p = static_cast<AudioCache*>(cookie);

    // ignore buffering messages
    switch (msg)
    {
    case MEDIA_ERROR:
        LOGE("Error %d, %d occurred", ext1, ext2);
        p->mError = ext1;
        break;
    case MEDIA_PREPARED:
        LOGV("prepared");
        break;
    case MEDIA_PLAYBACK_COMPLETE:
        LOGV("playback complete");
        break;
    default:
        LOGV("ignored");
        return;
    }

    // wake up thread
    Mutex::Autolock lock(p->mLock);
    p->mCommandComplete = true;
    p->mSignal.signal();
}

int MediaPlayerService::AudioCache::getSessionId()
{
    return 0;
}

void MediaPlayerService::addBatteryData(uint32_t params)
{
    Mutex::Autolock lock(mLock);

    int32_t time = systemTime() / 1000000L;

    // change audio output devices. This notification comes from AudioFlinger
    if ((params & kBatteryDataSpeakerOn)
            || (params & kBatteryDataOtherAudioDeviceOn)) {

        int deviceOn[NUM_AUDIO_DEVICES];
        for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
            deviceOn[i] = 0;
        }

        if ((params & kBatteryDataSpeakerOn)
                && (params & kBatteryDataOtherAudioDeviceOn)) {
            deviceOn[SPEAKER_AND_OTHER] = 1;
        } else if (params & kBatteryDataSpeakerOn) {
            deviceOn[SPEAKER] = 1;
        } else {
            deviceOn[OTHER_AUDIO_DEVICE] = 1;
        }

        for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
            if (mBatteryAudio.deviceOn[i] != deviceOn[i]){

                if (mBatteryAudio.refCount > 0) { // if playing audio
                    if (!deviceOn[i]) {
                        mBatteryAudio.lastTime[i] += time;
                        mBatteryAudio.totalTime[i] += mBatteryAudio.lastTime[i];
                        mBatteryAudio.lastTime[i] = 0;
                    } else {
                        mBatteryAudio.lastTime[i] = 0 - time;
                    }
                }

                mBatteryAudio.deviceOn[i] = deviceOn[i];
            }
        }
        return;
    }

    // an sudio stream is started
    if (params & kBatteryDataAudioFlingerStart) {
        // record the start time only if currently no other audio
        // is being played
        if (mBatteryAudio.refCount == 0) {
            for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
                if (mBatteryAudio.deviceOn[i]) {
                    mBatteryAudio.lastTime[i] -= time;
                }
            }
        }

        mBatteryAudio.refCount ++;
        return;

    } else if (params & kBatteryDataAudioFlingerStop) {
        if (mBatteryAudio.refCount <= 0) {
            LOGW("Battery track warning: refCount is <= 0");
            return;
        }

        // record the stop time only if currently this is the only
        // audio being played
        if (mBatteryAudio.refCount == 1) {
            for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
                if (mBatteryAudio.deviceOn[i]) {
                    mBatteryAudio.lastTime[i] += time;
                    mBatteryAudio.totalTime[i] += mBatteryAudio.lastTime[i];
                    mBatteryAudio.lastTime[i] = 0;
                }
            }
        }

        mBatteryAudio.refCount --;
        return;
    }

    int uid = IPCThreadState::self()->getCallingUid();
    if (uid == AID_MEDIA) {
        return;
    }
    int index = mBatteryData.indexOfKey(uid);

    if (index < 0) { // create a new entry for this UID
        BatteryUsageInfo info;
        info.audioTotalTime = 0;
        info.videoTotalTime = 0;
        info.audioLastTime = 0;
        info.videoLastTime = 0;
        info.refCount = 0;

        if (mBatteryData.add(uid, info) == NO_MEMORY) {
            LOGE("Battery track error: no memory for new app");
            return;
        }
    }

    BatteryUsageInfo &info = mBatteryData.editValueFor(uid);

    if (params & kBatteryDataCodecStarted) {
        if (params & kBatteryDataTrackAudio) {
            info.audioLastTime -= time;
            info.refCount ++;
        }
        if (params & kBatteryDataTrackVideo) {
            info.videoLastTime -= time;
            info.refCount ++;
        }
    } else {
        if (info.refCount == 0) {
            LOGW("Battery track warning: refCount is already 0");
            return;
        } else if (info.refCount < 0) {
            LOGE("Battery track error: refCount < 0");
            mBatteryData.removeItem(uid);
            return;
        }

        if (params & kBatteryDataTrackAudio) {
            info.audioLastTime += time;
            info.refCount --;
        }
        if (params & kBatteryDataTrackVideo) {
            info.videoLastTime += time;
            info.refCount --;
        }

        // no stream is being played by this UID
        if (info.refCount == 0) {
            info.audioTotalTime += info.audioLastTime;
            info.audioLastTime = 0;
            info.videoTotalTime += info.videoLastTime;
            info.videoLastTime = 0;
        }
    }
}

status_t MediaPlayerService::pullBatteryData(Parcel* reply) {
    Mutex::Autolock lock(mLock);

    // audio output devices usage
    int32_t time = systemTime() / 1000000L; //in ms
    int32_t totalTime;

    for (int i = 0; i < NUM_AUDIO_DEVICES; i++) {
        totalTime = mBatteryAudio.totalTime[i];

        if (mBatteryAudio.deviceOn[i]
            && (mBatteryAudio.lastTime[i] != 0)) {
                int32_t tmpTime = mBatteryAudio.lastTime[i] + time;
                totalTime += tmpTime;
        }

        reply->writeInt32(totalTime);
        // reset the total time
        mBatteryAudio.totalTime[i] = 0;
   }

    // codec usage
    BatteryUsageInfo info;
    int size = mBatteryData.size();

    reply->writeInt32(size);
    int i = 0;

    while (i < size) {
        info = mBatteryData.valueAt(i);

        reply->writeInt32(mBatteryData.keyAt(i)); //UID
        reply->writeInt32(info.audioTotalTime);
        reply->writeInt32(info.videoTotalTime);

        info.audioTotalTime = 0;
        info.videoTotalTime = 0;

        // remove the UID entry where no stream is being played
        if (info.refCount <= 0) {
            mBatteryData.removeItemsAt(i);
            size --;
            i --;
        }
        i++;
    }
    return NO_ERROR;
}
} // namespace android
