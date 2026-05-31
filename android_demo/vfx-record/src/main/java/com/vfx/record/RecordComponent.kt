package com.vfx.record

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.LinearLayout
import android.widget.Toast
import com.vfx.core.VfxEngine
import com.vfx.core.CodecType
import java.io.File

class RecordComponent @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private var isRecording = false
    private var vfxEngine: VfxEngine? = null
    private lateinit var recordButton: ImageButton

    init {
        LayoutInflater.from(context).inflate(R.layout.layout_record_component, this, true)

        recordButton = findViewById(R.id.btn_record)
        recordButton.setOnClickListener {
            if (isRecording) {
                stopRecording()
            } else {
                startRecording()
            }
        }

        // Bottom Panel Stub Actions
        findViewById<LinearLayout>(R.id.btn_effects).setOnClickListener {
            Toast.makeText(context, "Effects (TODO: Show bottom sheet with VFX list)", Toast.LENGTH_SHORT).show()
        }

        findViewById<LinearLayout>(R.id.btn_gallery).setOnClickListener {
            Toast.makeText(context, "Gallery (TODO: Open system picker)", Toast.LENGTH_SHORT).show()
        }
    }

    fun attachEngine(engine: VfxEngine) {
        this.vfxEngine = engine
    }

    private fun startRecording() {
        val outputFile = File(context.cacheDir, "vfx_record_out.mp4")

        // Hardcode H.265 for performance demonstration
        vfxEngine?.startRecording(outputFile.absolutePath, CodecType.H265)
        isRecording = true

        // Morph the button into a Stop square
        recordButton.setBackgroundResource(R.drawable.bg_record_btn_recording)
        val params = recordButton.layoutParams
        params.width = (48 * resources.displayMetrics.density).toInt()
        params.height = (48 * resources.displayMetrics.density).toInt()
        recordButton.layoutParams = params
    }

    private fun stopRecording() {
        vfxEngine?.stopRecording()
        isRecording = false

        // Morph back to circular capture
        recordButton.setBackgroundResource(R.drawable.bg_record_btn_idle)
        val params = recordButton.layoutParams
        params.width = (72 * resources.displayMetrics.density).toInt()
        params.height = (72 * resources.displayMetrics.density).toInt()
        recordButton.layoutParams = params

        Toast.makeText(context, "Video Saved!", Toast.LENGTH_SHORT).show()
    }
}
