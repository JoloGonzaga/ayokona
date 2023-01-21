// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
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
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>
#include <time.h>

#include "Face_detection/face.h"

#include "Face_detection/ndkcamera.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

using namespace std::chrono;

std::vector<Object> faceobjects;
JavaVM* gJvm = nullptr;
static jclass gClass;
static jobject gClassLoader;
static jmethodID gFindClassMethod;

JNIEnv* getEnv() {
    JNIEnv *env;
    int status = gJvm->GetEnv((void**)&env, JNI_VERSION_1_4);
    if(status < 0) {
        status = gJvm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return nullptr;
        }
    }
    return env;
}

jclass findClass(const char* name) {
    return static_cast<jclass>(getEnv()->CallObjectMethod(gClassLoader, gFindClassMethod, getEnv()->NewStringUTF(name)));
}

static int draw_unsupported(cv::Mat& rgb)
{
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                    cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

static int draw_fps(cv::Mat& rgb)
{
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f)
        {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--)
        {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f)
        {
            return 0;
        }

        for (int i = 0; i < 10; i++)
        {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                    cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

time_t timeSecStart = 0;
static int draw_alert(cv::Mat& rgb){
    if(faceobjects.size() == 1){
        float avgEAR = (faceobjects[0].earright + faceobjects[0].earleft) / 2;
        float threshold = 0.35f;
        if(avgEAR < threshold){
            if(timeSecStart == NULL){
                time(&timeSecStart);
            }
            time_t timeSecNow;
            time(&timeSecNow);
            int sec = (int)timeSecNow - (int)timeSecStart;
            //__android_log_print(ANDROID_LOG_DEBUG, "alertTime", "time %i", sec);
            if(sec > 2){
                __android_log_print(ANDROID_LOG_DEBUG, "alertTimeNotif", "ALERT");
                faceobjects[0].isAlert = true;
                const char text[] = "ALERT";

                int baseLine = 0;
                cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

                int y = (rgb.rows - label_size.height) / 2;
                int x = (rgb.cols - label_size.width) / 2;

                cv::rectangle(rgb, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                              cv::Scalar(255, 0, 0), -1);

                cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255));
            }
        }else{
            time(&timeSecStart);
            faceobjects[0].isAlert = false;
        }

    }
    return 0;
}

static Face* g_blazeface = 0;
static ncnn::Mutex lock;

class MyNdkCamera : public NdkCameraWindow
{
public:
    virtual void on_image_render(cv::Mat& rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat& rgb) const
{
    {
        ncnn::MutexLockGuard g(lock);

        if (g_blazeface)
        {
            high_resolution_clock::time_point t1 = high_resolution_clock::now();
            g_blazeface->detect(rgb, faceobjects);
            high_resolution_clock::time_point t2 = high_resolution_clock::now();

            if(faceobjects.size() > 0){
                duration<double, std::milli> time_span = t1 - t2;
                __android_log_print(ANDROID_LOG_DEBUG, "TimeFace","MiliSegundo: %s", std::to_string(time_span.count()).c_str());
                g_blazeface->draw(rgb, faceobjects);
            }

            draw_alert(rgb);
        }
        else
        {
            draw_unsupported(rgb);
        }
    }
    draw_fps(rgb);
}

static MyNdkCamera* g_camera = 0;

extern "C" {

    JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnLoad");

        gJvm = vm;  // cache the JavaVM pointer
        auto env = getEnv();
        //replace with one of your classes in the line below
        auto randomClass = env->FindClass("com/vyw/tflite/BlazeFaceNcnn");
        gClass = (jclass)env->NewGlobalRef(randomClass);
        jclass classClass = env->GetObjectClass(randomClass);
        auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
        auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                                     "()Ljava/lang/ClassLoader;");
        gClassLoader = env->CallObjectMethod(randomClass, getClassLoaderMethod);
        gFindClassMethod = env->GetMethodID(classLoaderClass, "findClass",
                                            "(Ljava/lang/String;)Ljava/lang/Class;");


        g_camera = new MyNdkCamera;

        return JNI_VERSION_1_4;
    }

    JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnUnload");

        {
            ncnn::MutexLockGuard g(lock);

            delete g_blazeface;
            g_blazeface = 0;
        }

        delete g_camera;
        g_camera = 0;
    }

    // public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
    JNIEXPORT jboolean JNICALL Java_com_vyw_tflite_BlazeFaceNcnn_loadModel(JNIEnv* env, jobject thiz, jobject assetManager, jint modelid, jint cpugpu)
    {
        if (modelid < 0 || modelid > 6 || cpugpu < 0 || cpugpu > 1)
        {
            return JNI_FALSE;
        }

        AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);
        const char* modeltypes[] =
        {
            "blazeface",
            "blazeface",
            "blazeface"
        };
        const int target_sizes[] =
        {
            192,
            320,
            640
        };
        const char* modeltype = modeltypes[(int)modelid];
        int target_size = target_sizes[(int)modelid];
        bool use_gpu = (int)cpugpu == 1;

        char modelFinal[256];
        sprintf(modelFinal, "Models/%s", modeltype);
        __android_log_print(ANDROID_LOG_DEBUG, "ncnnModel", "loadModel %s", modelFinal);
        // reload
        {
            ncnn::MutexLockGuard g(lock);

            if (use_gpu && ncnn::get_gpu_count() == 0)
            {
                // no gpu
                delete g_blazeface;
                g_blazeface = 0;
            }
            else
            {
                if (!g_blazeface)
                    g_blazeface = new Face;
                g_blazeface->load(mgr, modelFinal,target_size, use_gpu);
            }
        }

        return JNI_TRUE;
    }

    // public native boolean openCamera(int facing);
    JNIEXPORT jboolean JNICALL Java_com_vyw_tflite_BlazeFaceNcnn_openCamera(JNIEnv* env, jobject thiz, jint facing)
    {
        if (facing < 0 || facing > 1)
            return JNI_FALSE;

        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "openCamera %d", facing);

        g_camera->open((int)facing);

        return JNI_TRUE;
    }

    // public native boolean closeCamera();
    JNIEXPORT jboolean JNICALL Java_com_vyw_tflite_BlazeFaceNcnn_closeCamera(JNIEnv* env, jobject thiz)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "closeCamera");

        g_camera->close();

        return JNI_FALSE;
    }

    // public native boolean setOutputWindow(Surface surface);
    JNIEXPORT jboolean JNICALL Java_com_vyw_tflite_BlazeFaceNcnn_setOutputWindow(JNIEnv* env, jobject thiz, jobject surface)
    {
        ANativeWindow* win = ANativeWindow_fromSurface(env, surface);

        __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

        g_camera->set_window(win);

        return JNI_TRUE;
    }
    JNIEXPORT jstring JNICALL
    Java_com_vyw_tflite_BlazeFaceNcnn_alertTrigger(JNIEnv *env, jobject thiz) {
        jstring jstr = getEnv()->NewStringUTF((faceobjects.size() > 0)?std::to_string(faceobjects[0].isAlert).c_str():"NO FACE");
        jmethodID sensor = getEnv()->GetMethodID(gClass, "alertSensor",
                                                 "(Ljava/lang/String;)Ljava/lang/String;");

        jobject result = getEnv()->CallObjectMethod(thiz, sensor, jstr);
        const char* str = getEnv()->GetStringUTFChars((jstring) result, NULL); // should be released but what a heck, it's a tutorial :)

        return getEnv()->NewStringUTF(str);
    }
}

