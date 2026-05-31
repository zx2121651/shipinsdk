package com.vfx.demo

import android.os.Bundle
import android.widget.ImageView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.vfx.preview.PreviewComponent
import com.vfx.record.RecordComponent

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Inflate the new CapCut-styled XML layout
        setContentView(R.layout.activity_main)

        val previewComponent = findViewById<PreviewComponent>(R.id.previewComponent)
        val recordComponent = findViewById<RecordComponent>(R.id.recordComponent)

        // Wire up engine
        recordComponent.attachEngine(previewComponent.getEngine())

        // Top Bar Stub Actions
        findViewById<ImageView>(R.id.btn_close).setOnClickListener {
            finish()
        }

        // Right Sidebar Stub Actions
        findViewById<ImageView>(R.id.btn_flip).setOnClickListener {
            Toast.makeText(this, "Flip Camera (TODO: Update C++ uniform)", Toast.LENGTH_SHORT).show()
        }

        findViewById<ImageView>(R.id.btn_timer).setOnClickListener {
            Toast.makeText(this, "Timer (TODO: Add 3s countdown)", Toast.LENGTH_SHORT).show()
        }

        findViewById<ImageView>(R.id.btn_filter).setOnClickListener {
            Toast.makeText(this, "Filters (TODO: Swap RenderGraph node)", Toast.LENGTH_SHORT).show()
        }

        findViewById<ImageView>(R.id.btn_beauty).setOnClickListener {
            Toast.makeText(this, "Beauty (TODO: Enable Skin Smoothing pass)", Toast.LENGTH_SHORT).show()
        }
    }
}
