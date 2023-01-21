package com.vyw.tflite

import android.content.Intent
import android.media.MediaPlayer
import android.os.Bundle
import android.view.View
import android.view.Window
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import com.vyw.tflite.databinding.ActivityMainBinding


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        supportActionBar?.hide();
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
    }

    fun btnClick(view: View) {
        val intent = Intent(this, CameraStarter::class.java)
        startActivity(intent)
    }

    fun about_btnClick(view: View) {
        val intent = Intent(this, About::class.java)
        startActivity(intent)
    }

    fun settings_btnClick(view: View) {
        val intent = Intent(this, settings_activity::class.java)
        startActivity(intent)
    }


}
