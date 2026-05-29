package com.vfx.record

import android.content.Context
import android.util.AttributeSet
import android.widget.Button
import android.widget.LinearLayout
import com.vfx.core.VfxEngine
import com.vfx.core.CodecType
import java.io.File

class RecordComponent @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val recordButton = Button(context)
    private var isRecording = false
    private var vfxEngine: VfxEngine? = null

    init {
        orientation = HORIZONTAL
        recordButton.text = "Start Record (H.265)"

        recordButton.setOnClickListener {
            if (isRecording) {
                stopRecording()
            } else {
                startRecording()
            }
        }

        addView(recordButton)
    }

    fun attachEngine(engine: VfxEngine) {
        this.vfxEngine = engine
    }

    private fun startRecording() {
        val outputFile = File(context.cacheDir, "vfx_record_out.mp4")

        // Pass H.265 specifically
        vfxEngine?.startRecording(outputFile.absolutePath, CodecType.H265)

        isRecording = true
        recordButton.text = "Stop Record"
    }

    private fun stopRecording() {
        vfxEngine?.stopRecording()

        isRecording = false
        recordButton.text = "Start Record (H.265)"
    }
}
