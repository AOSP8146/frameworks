/*
**
** Copyright 2013, The Android Open Source Project
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

// #define LOG_NDEBUG 0
#define LOG_TAG "CameraRequest"
#include <utils/Log.h>

#include <camera/camera2/CaptureRequest.h>

#include <binder/Parcel.h>
#include <gui/Surface.h>
#include <gui/view/Surface.h>

namespace android {
namespace hardware {
namespace camera2 {

// These must be in the .cpp (to avoid inlining)
CaptureRequest::CaptureRequest() = default;
CaptureRequest::~CaptureRequest() = default;
CaptureRequest::CaptureRequest(const CaptureRequest& rhs) = default;
CaptureRequest::CaptureRequest(CaptureRequest&& rhs) noexcept = default;


status_t CaptureRequest::readFromParcel(const android::Parcel* parcel) {
    if (parcel == NULL) {
        ALOGE("%s: Null parcel", __FUNCTION__);
        return BAD_VALUE;
    }

    mMetadata.clear();
    mSurfaceList.clear();

    status_t err = OK;

    if ((err = mMetadata.readFromParcel(parcel)) != OK) {
        ALOGE("%s: Failed to read metadata from parcel", __FUNCTION__);
        return err;
    }
    ALOGV("%s: Read metadata from parcel", __FUNCTION__);

    int32_t size;
    if ((err = parcel->readInt32(&size)) != OK) {
        ALOGE("%s: Failed to read surface list size from parcel", __FUNCTION__);
        return err;
    }
    ALOGV("%s: Read surface list size = %d", __FUNCTION__, size);

    // Do not distinguish null arrays from 0-sized arrays.
    for (int i = 0; i < size; ++i) {
        // Parcel.writeParcelableArray
        size_t len;
        const char16_t* className = parcel->readString16Inplace(&len);
        ALOGV("%s: Read surface class = %s", __FUNCTION__,
              className != NULL ? String8(className).string() : "<null>");

        if (className == NULL) {
            continue;
        }

        // Surface.writeToParcel
        view::Surface surfaceShim;
        if ((err = surfaceShim.readFromParcel(parcel)) != OK) {
            ALOGE("%s: Failed to read output target Surface %d from parcel: %s (%d)",
                    __FUNCTION__, i, strerror(-err), err);
            return err;
        }

        sp<Surface> surface;
        if (surfaceShim.graphicBufferProducer != NULL) {
            surface = new Surface(surfaceShim.graphicBufferProducer);
        }

        mSurfaceList.push_back(surface);
    }

    int isReprocess = 0;
    if ((err = parcel->readInt32(&isReprocess)) != OK) {
        ALOGE("%s: Failed to read reprocessing from parcel", __FUNCTION__);
        return err;
    }
    mIsReprocess = (isReprocess != 0);

    return OK;
}

status_t CaptureRequest::writeToParcel(android::Parcel* parcel) const {
    if (parcel == NULL) {
        ALOGE("%s: Null parcel", __FUNCTION__);
        return BAD_VALUE;
    }

    status_t err = OK;

    if ((err = mMetadata.writeToParcel(parcel)) != OK) {
        return err;
    }

    int32_t size = static_cast<int32_t>(mSurfaceList.size());

    // Send 0-sized arrays when it's empty. Do not send null arrays.
    parcel->writeInt32(size);

    for (int32_t i = 0; i < size; ++i) {
        // not sure if readParcelableArray does this, hard to tell from source
        parcel->writeString16(String16("android.view.Surface"));

        // Surface.writeToParcel
        view::Surface surfaceShim;
        surfaceShim.name = String16("unknown_name");
        surfaceShim.graphicBufferProducer = mSurfaceList[i]->getIGraphicBufferProducer();
        if ((err = surfaceShim.writeToParcel(parcel)) != OK) {
            ALOGE("%s: Failed to write output target Surface %d to parcel: %s (%d)",
                    __FUNCTION__, i, strerror(-err), err);
            return err;
        }
    }

    parcel->writeInt32(mIsReprocess ? 1 : 0);

    return OK;
}

} // namespace camera2
} // namespace hardware
} // namespace android
