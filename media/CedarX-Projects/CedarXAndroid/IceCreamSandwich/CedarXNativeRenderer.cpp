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

#define LOG_TAG "CedarXNativeRenderer"
#include <utils/Log.h>

#include "CedarXNativeRenderer.h"

#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapPmem.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>
#include <hardware/hwcomposer.h>

namespace android {

CedarXNativeRenderer::CedarXNativeRenderer(
        const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mNativeWindow(nativeWindow) {

    int32_t halFormat;
    size_t bufWidth, bufHeight;

    CHECK(meta->findInt32(kKeyColorFormat, &halFormat));
    CHECK(meta->findInt32(kKeyWidth, &mWidth));
    CHECK(meta->findInt32(kKeyHeight, &mHeight));

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    bufWidth = mWidth;
    bufHeight = mHeight;

    CHECK(mNativeWindow != NULL);

    CHECK_EQ(0,
            native_window_set_usage(
            mNativeWindow.get(),
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0,
            native_window_set_scaling_mode(
            mNativeWindow.get(),
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

    // Width must be multiple of 32???
    CHECK_EQ(0, native_window_set_buffers_geometry(
                mNativeWindow.get(),
                bufWidth,
                bufHeight,
                halFormat));

    uint32_t transform;
    switch (rotationDegrees) {
        case 0: transform = 0; break;
        case 90: transform = HAL_TRANSFORM_ROT_90; break;
        case 180: transform = HAL_TRANSFORM_ROT_180; break;
        case 270: transform = HAL_TRANSFORM_ROT_270; break;
        default: transform = 0; break;
    }

    if (transform) {
        CHECK_EQ(0, native_window_set_buffers_transform(
                    mNativeWindow.get(), transform));
    }
}

CedarXNativeRenderer::~CedarXNativeRenderer() {
}

void CedarXNativeRenderer::render(
        const void *data, size_t size, void *platformPrivate) {

    mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, HWC_LAYER_SETFRAMEPARA, (uint32_t)data);
}

int CedarXNativeRenderer::control(int cmd, int para) {

	switch(cmd){
	case VIDEORENDER_CMD_GETFRAMEID:
		return mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_SETPARAMETER, HWC_LAYER_GETCURFRAMEPARA, 0);
	default:
		break;
	}

    return 0;
}

}  // namespace android
