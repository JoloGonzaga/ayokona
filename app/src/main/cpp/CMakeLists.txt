# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("tflite")

# opencv
set(OpenCV_STATIC ON)
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/cppLibs/OpenCV-android-sdk/sdk/native/jni)
find_package (OpenCV REQUIRED core imgproc)

#NCNN library
set(ncnn_DIR ${CMAKE_SOURCE_DIR}/cppLibs/ncnn-20221128-android-vulkan/${ANDROID_ABI}/lib/cmake/ncnn)
find_package(ncnn REQUIRED)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

    # Sets the library as a shared library.
        SHARED

    # Provides a relative path to your source file(s).
        blazefacencnn.cpp
        Face_detection/ndkcamera.cpp
        Face_detection/face.cpp
        Face_detection/landmark.cpp
        )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib
        ncnn
        camera2ndk
        mediandk
        ${OpenCV_LIBS}
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})