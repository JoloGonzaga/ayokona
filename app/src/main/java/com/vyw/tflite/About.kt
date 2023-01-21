package com.vyw.tflite

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.vyw.tflite.R
import android.widget.TextView
import android.content.Intent
import android.widget.ImageView
import com.vyw.tflite.MainActivity

class About : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_about)
        val return_icon = findViewById<ImageView>(R.id.back)
        val title = findViewById<TextView>(R.id.toolbar_title)
        return_icon.setOnClickListener {
            startActivity(
                Intent(
                    this@About,
                    MainActivity::class.java
                )
            )
        }
    }
}