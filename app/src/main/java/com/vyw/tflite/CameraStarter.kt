package com.vyw.tflite

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.View
import android.widget.AdapterView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.vyw.tflite.databinding.ActivityCameraStarterBinding

class CameraStarter : Activity(), SurfaceHolder.Callback{

    private val REQUEST_CAMERA = 100

    private lateinit var binding : ActivityCameraStarterBinding
    private var blazefacecnn = BlazeFaceNcnn()
    private val facing = 0
    private var currentModel = 0
    private var currentCPUGPU = 0
    private var isCameraOpen : Boolean = false;
    val alert = Thread {
//        while (isCameraOpen) {
//            blazefacecnn.alertTrigger()
//        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityCameraStarterBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.cameraview!!.holder.setFormat(PixelFormat.RGBA_8888)
        binding.cameraview!!.holder.addCallback(this)

        binding.spinnerModel!!.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(
                arg0: AdapterView<*>? ,
                arg1: View ,
                position: Int ,
                id: Long
            ) {
                if (position != currentModel) {
                    currentModel = position
                    reload()
                }
            }

            override fun onNothingSelected(arg0: AdapterView<*>?) {}
        }
        reload()
    }

    private fun startThread(thread: Thread) {
        thread.start()
    }

    private fun reload() {
        val ret_init: Boolean = blazefacecnn.loadModel(assets, currentModel , currentCPUGPU)
        if (!ret_init) {
            Log.e("MainActivity" , "blazefacecnn loaodModel failed")
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        isCameraOpen = blazefacecnn.openCamera(facing)
        startThread(alert)
    }
    override fun surfaceChanged(holder: SurfaceHolder , format: Int , width: Int , height: Int) {
        blazefacecnn.setOutputWindow(holder.surface)
    }
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        alert.interrupt()
        isCameraOpen = blazefacecnn.closeCamera()
        Log.d("HatdogCallback" , "doghat destroyed")
    }

    //Overrides Activity() function
    override fun onResume() {
        super.onResume()

        if(ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED){
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        }
    }
    override fun onDestroy() {
        super.onDestroy()
        isCameraOpen = blazefacecnn.closeCamera()
        alert.interrupt()
    }
}