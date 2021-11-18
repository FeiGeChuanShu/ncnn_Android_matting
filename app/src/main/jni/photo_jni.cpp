// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <android/asset_manager_jni.h>
#include <android/bitmap.h>
#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

// ncnn
#include "net.h"
#include "benchmark.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "bg.h"
static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::PoolAllocator g_workspace_pool_allocator;

static ncnn::Net bgnet[2];


extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_DEBUG, "photo", "JNI_OnLoad");

    ncnn::create_gpu_instance();

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    __android_log_print(ANDROID_LOG_DEBUG, "photo", "JNI_OnUnload");

    ncnn::destroy_gpu_instance();
}

// public native boolean Init(AssetManager mgr);
JNIEXPORT jboolean JNICALL Java_com_tencent_photo_Photo_Init(JNIEnv* env, jobject thiz, jobject assetManager)
{

    ncnn::Option opt;
    opt.lightmode = true;
    opt.num_threads = 4;
    opt.blob_allocator = &g_blob_pool_allocator;
    opt.workspace_allocator = &g_workspace_pool_allocator;

    // use vulkan compute
    if (ncnn::get_gpu_count() != 0)
        opt.use_vulkan_compute = true;

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    const char* model_paths[2] = {"mobilenetv2.bin", "hrnet-w18.bin"};
    const char* param_paths[2] = {"mobilenetv2.param", "hrnet-w18.param"};
    for (int i=0; i<2; i++)
    {
        bgnet[i].opt = opt;

        int ret0 = bgnet[i].load_param(mgr, param_paths[i]);
        int ret1 = bgnet[i].load_model(mgr, model_paths[i]);
        __android_log_print(ANDROID_LOG_DEBUG, "photo", "%d,%d", ret0, ret1);
    }
    return JNI_TRUE;
}

// public native Bitmap StyleTransfer(Bitmap bitmap, int style_type, boolean use_gpu);
JNIEXPORT jboolean JNICALL Java_com_tencent_photo_Photo_Process(JNIEnv* env, jobject thiz, jobject bitmap, jint style_type, jboolean use_gpu)
{
    if (style_type < 0 || style_type >= 5)
        return JNI_FALSE;

    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0)
        return JNI_FALSE;

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return JNI_FALSE;

    ncnn::Mat in = ncnn::Mat::from_android_bitmap(env, bitmap, ncnn::Mat::PIXEL_RGB);
    cv::Mat rgb = cv::Mat::zeros(in.h,in.w,CV_8UC3);
    in.to_pixels(rgb.data, ncnn::Mat::PIXEL_RGB);

    int width = rgb.cols;
    int height = rgb.rows;
    ncnn::Mat in_resize = ncnn::Mat::from_pixels_resize(rgb.data, ncnn::Mat::PIXEL_RGB, rgb.cols,rgb.rows,512,512);
    const float meanVals[3] = { 127.5f, 127.5f,  127.5f };
    const float normVals[3] = { 0.0078431f, 0.0078431f, 0.0078431f };
    in_resize.substract_mean_normalize(meanVals, normVals);
    ncnn::Mat out;
    {
        ncnn::Extractor ex = bgnet[style_type].create_extractor();
        ex.set_vulkan_compute(use_gpu);
        ex.input("input", in_resize);
        ex.extract("output", out);
    }

    ncnn::Mat alpha;
    ncnn::resize_bilinear(out,alpha,width,height);
    cv::Mat blendImg = cv::Mat::zeros(cv::Size(width,height), CV_8UC3);

    const int bg_color[3] = {120, 255, 155};
    float* alpha_data = (float*)alpha.data;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            float alpha_ = alpha_data[i*width+j];
            blendImg.at < cv::Vec3b>(i, j)[0] = rgb.at < cv::Vec3b>(i, j)[0] * alpha_ + (1 - alpha_) * bg_color[0];
            blendImg.at < cv::Vec3b>(i, j)[1] = rgb.at < cv::Vec3b>(i, j)[1] * alpha_ + (1 - alpha_) * bg_color[1];
            blendImg.at < cv::Vec3b>(i, j)[2] = rgb.at < cv::Vec3b>(i, j)[2] * alpha_ + (1 - alpha_) * bg_color[2];
        }
    }

    ncnn::Mat blengImg_ncnn = ncnn::Mat::from_pixels(blendImg.data,ncnn::Mat::PIXEL_RGB,blendImg.cols,blendImg.rows);

    // ncnn to bitmap
    blengImg_ncnn.to_android_bitmap(env, bitmap, ncnn::Mat::PIXEL_RGB);

    double elasped = ncnn::get_current_time() - start_time;
    __android_log_print(ANDROID_LOG_DEBUG, "photo", "%.2fms", elasped);

    return JNI_TRUE;
}

}
