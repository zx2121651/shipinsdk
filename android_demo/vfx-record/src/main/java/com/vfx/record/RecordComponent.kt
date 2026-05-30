package com.vfx.record

import android.content.Context
import android.graphics.Color
import android.util.AttributeSet
import android.view.Gravity
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.LinearLayout
import com.vfx.core.VfxEngine
import com.vfx.core.CodecType
import java.io.File

class RecordComponent @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private val recordButton = ImageButton(context)
    private var isRecording = false
    private var vfxEngine: VfxEngine? = null

    init {
        // Make the component background transparent to float over the preview
        setBackgroundColor(Color.TRANSPARENT)

        // Setup modern circular record button
        recordButton.setBackgroundResource(R.drawable.bg_record_btn_idle)
        recordButton.elevation = 8f

        val params = LayoutParams(
            (72 * resources.displayMetrics.density).toInt(),
            (72 * resources.displayMetrics.density).toInt()
        ).apply {
            gravity = Gravity.CENTER
            bottomMargin = (32 * resources.displayMetrics.density).toInt()
        }

        recordButton.layoutParams = params

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

        // Animate/Transition to a square "stop" icon
        recordButton.setBackgroundResource(R.drawable.bg_record_btn_recording)
        val params = recordButton.layoutParams
        params.width = (48 * resources.displayMetrics.density).toInt()
        params.height = (48 * resources.displayMetrics.density).toInt()
        recordButton.layoutParams = params
    }

    private fun stopRecording() {
        vfxEngine?.stopRecording()

        isRecording = false

        // Revert to circular icon
        recordButton.setBackgroundResource(R.drawable.bg_record_btn_idle)
        val params = recordButton.layoutParams
        params.width = (72 * resources.displayMetrics.density).toInt()
        params.height = (72 * resources.displayMetrics.density).toInt()
        recordButton.layoutParams = params
    }
}
